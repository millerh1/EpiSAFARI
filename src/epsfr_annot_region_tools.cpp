#include <stdio.h>
#include <stdlib.h>
#include "epsfr_annot_region_tools.h"
#include <algorithm>
#include <string.h>
#include <math.h>
#include "epsfr_utils.h"
#include "epsfr_ansi_string.h"
#include "epsfr_nomenclature.h"
#include "epsfr_genomics_coords.h"

#define MIN(x,y) ((x) < (y)?(x):(y))
#define MAX(x,y) ((x) > (y)?(x):(y))

bool __DUMP_ANNOT_REGION_TOOLS_MSGS__ = false;

void sort_set_sorting_info(vector<t_annot_region*>* regions, bool (sort_regions_callback)(t_annot_region*, t_annot_region*))
{
	// Set the sorting info for each region.
	if(regions->size() == 0)
	{
		return;
	}

	// Sort the regions.
	if(sort_regions_callback != NULL)
	{
		sort(regions->begin(), regions->end(), sort_regions_callback);
	}

		// Set the cumulative start positions for each region: Cumulative start from the end of the list.
	int current_cumulative_start = regions->back()->start;
	for(int i_reg = regions->size()-1; i_reg >= 0; i_reg--)
	{
		t_sorting_info* cur_reg_sorting_info = new t_sorting_info();
		if(current_cumulative_start > regions->at(i_reg)->start)
		{
			current_cumulative_start = regions->at(i_reg)->start;	
		}

		cur_reg_sorting_info->cumulative_sorted_start = current_cumulative_start;

		regions->at(i_reg)->sort_info = cur_reg_sorting_info;
	} // i_reg loop.

	// Set the cumulative end: Cumulative end from the beginning of the list.
	int current_cumulative_end = 0;
	for(int i_reg = 0; i_reg < (int)regions->size(); i_reg++)
	{
		//t_sorting_info* cur_reg_sorting_info = new t_sorting_info();
		t_sorting_info* cur_reg_sorting_info = regions->at(i_reg)->sort_info;
		if(current_cumulative_end < regions->at(i_reg)->end)
		{
			current_cumulative_end = regions->at(i_reg)->end;	
		}

		cur_reg_sorting_info->cumulative_sorted_end = current_cumulative_end;
	} // i_reg loop.
}

void delete_sorting_information(vector<t_annot_region*>* regions)
{
	// Delete all the sorting information for all the reads.
	for(int i_reg = 0; i_reg < (int)regions->size(); i_reg++)
	{
		delete regions->at(i_reg)->sort_info;
	}
}

/*
Extend bed: Depending on the strand of the regions, extend to 5' and/or 3'.
For regions on neg. strand, the 3p is on the left side. The coordinates for regions are, however, always with respect to the beginning of the forward strand.
*/
void extend_BED(vector<t_annot_region*>* regions, int l_extend, bool extend_3p, bool extend_5p)
{	
	for(int i = 0; i < (int)regions->size(); i++)
	{
		int l_2_extend = (l_extend>0)?(l_extend):(regions->at(i)->end-regions->at(i)->start);

		if(regions->at(i)->strand == '+')
		{
			if(extend_5p)
			{
				regions->at(i)->start = (regions->at(i)->start > l_2_extend)?(regions->at(i)->start - l_2_extend):(1);
			}
			
			if(extend_3p)
			{
				regions->at(i)->end = (regions->at(i)->end + l_2_extend);
			}
		} // strand check for pos. strand.
		else
		{
			if(extend_3p)
			{
				regions->at(i)->start = (regions->at(i)->start > l_2_extend)?(regions->at(i)->start - l_2_extend):(1);
			}
			
			if(extend_5p)
			{
				regions->at(i)->end = (regions->at(i)->end + l_2_extend);
			}
		} // strand check for neg. strand.
	} // i loop.
}

/*
Make sure that the external data structure pointers are correctly set for merging/intersecting operations between the newly allocated regions. This can be ensured by not reallocating
new regions while restructuring the regions per strands/chromosomes. restructure_annot_regions function may be very useful for this.
*/
void dump_BED(const char* bed_fp, vector<t_annot_region*>* annot_regions)
{
	FILE* f_bed = open_f(bed_fp, "w");

	for(int i_reg = 0; i_reg < (int)annot_regions->size(); i_reg++)
	{
		if(annot_regions->at(i_reg)->strand == '-' ||
			annot_regions->at(i_reg)->strand == '+')
		{
			if(annot_regions->at(i_reg)->name == NULL)
			{
				annot_regions->at(i_reg)->name = new char[5];
				strcpy(annot_regions->at(i_reg)->name, ".");
			}

			// Translate the start and end to CODEBASE's start and end.
			fprintf(f_bed, "%s\t%d\t%d\t%s\t.\t%c\n", annot_regions->at(i_reg)->chrom, 
				translate_coord(annot_regions->at(i_reg)->start, CODEBASE_COORDS::start_base, BED_COORDS::start_base),
				translate_coord(annot_regions->at(i_reg)->end, CODEBASE_COORDS::end_base, BED_COORDS::end_base),
				annot_regions->at(i_reg)->name,
				annot_regions->at(i_reg)->strand);
		}
		else
		{
			fprintf(f_bed, "%s\t%d\t%d\n", annot_regions->at(i_reg)->chrom, 
				translate_coord(annot_regions->at(i_reg)->start, CODEBASE_COORDS::start_base, BED_COORDS::start_base),
				translate_coord(annot_regions->at(i_reg)->end, CODEBASE_COORDS::end_base, BED_COORDS::end_base));
		}
	} // i_ref loop.

	fclose(f_bed);
}

void delete_restructured_annot_regions(t_restr_annot_region_list* restructured_region_lists)
{
	for(int i_chr = 0; i_chr < (int)restructured_region_lists->chr_ids->size(); i_chr++)
	{
		delete [] restructured_region_lists->chr_ids->at(i_chr);
		delete restructured_region_lists->neg_strand_regions_per_chrom[i_chr];
		delete restructured_region_lists->pos_strand_regions_per_chrom[i_chr];
		delete restructured_region_lists->regions_per_chrom[i_chr];
	} // i_chr loop.

	delete [] restructured_region_lists->neg_strand_regions_per_chrom;
	delete [] restructured_region_lists->pos_strand_regions_per_chrom;
	delete [] restructured_region_lists->regions_per_chrom;
	delete restructured_region_lists->chr_ids;

	delete restructured_region_lists;
}

t_restr_annot_region_list* restructure_annot_regions(vector<t_annot_region*>* regions)
{
	// This is the list of regions per chromosome.
	vector<char*>* chr_ids = get_chr_ids(regions);
	t_restr_annot_region_list* region_lists = new t_restr_annot_region_list();

	// The chromosome id list is built up dynamically in the region list loop.
	//region_lists->chr_ids = new vector<char*>();
	region_lists->chr_ids = chr_ids;
	region_lists->neg_strand_regions_per_chrom = new vector<t_annot_region*>*[(int)chr_ids->size() + 2];
	region_lists->pos_strand_regions_per_chrom = new vector<t_annot_region*>*[(int)chr_ids->size() + 2];
	region_lists->regions_per_chrom = new vector<t_annot_region*>*[(int)chr_ids->size() + 2];

	for(int i_chr = 0; i_chr < (int)chr_ids->size(); i_chr++)
	{
		region_lists->pos_strand_regions_per_chrom[i_chr] = new vector<t_annot_region*>();
		region_lists->neg_strand_regions_per_chrom[i_chr] = new vector<t_annot_region*>();
		region_lists->regions_per_chrom[i_chr] = new vector<t_annot_region*>();
	} // i_chr loop.

	// Go over all the regions and separate them into appropriate list.
	for(int i_reg = 0; i_reg < (int)regions->size(); i_reg++)
	{
		int i_chr = t_string::get_i_str(chr_ids, regions->at(i_reg)->chrom);

		if(i_chr < (int)chr_ids->size())
		{
			region_lists->regions_per_chrom[i_chr]->push_back(regions->at(i_reg));

			if(regions->at(i_reg)->strand == '+')
			{
				region_lists->pos_strand_regions_per_chrom[i_chr]->push_back(regions->at(i_reg));
			}
			else if(regions->at(i_reg)->strand == '-')
			{
				region_lists->neg_strand_regions_per_chrom[i_chr]->push_back(regions->at(i_reg));
			}
			else
			{
				printf("Unknown strand char %c @ %s(%d).\n", regions->at(i_reg)->strand, __FILE__, __LINE__);
				exit(0);
			}
		}
	} // i_reg loop.

	// Sort the regions per chromosome and all the regions.
	for(int i_chr = 0; i_chr < (int)chr_ids->size(); i_chr++)
	{
		sort(region_lists->pos_strand_regions_per_chrom[i_chr]->begin(), region_lists->pos_strand_regions_per_chrom[i_chr]->end(), sort_regions);
		sort(region_lists->neg_strand_regions_per_chrom[i_chr]->begin(), region_lists->neg_strand_regions_per_chrom[i_chr]->end(), sort_regions);
		sort(region_lists->regions_per_chrom[i_chr]->begin(), region_lists->regions_per_chrom[i_chr]->end(), sort_regions);
	} // i_chr loop.

	// Finally sort the regions.
	return(region_lists);
}

t_restr_annot_region_list* restructure_annot_regions(vector<t_annot_region*>* regions, vector<char*>* chr_ids)
{
	// This is the list of regions per chromosome.
	t_restr_annot_region_list* region_lists = new t_restr_annot_region_list();

	region_lists->chr_ids = NULL; // Do not free the memory if it is null.
	region_lists->neg_strand_regions_per_chrom = new vector<t_annot_region*>*[(int)chr_ids->size() + 2];
	region_lists->pos_strand_regions_per_chrom = new vector<t_annot_region*>*[(int)chr_ids->size() + 2];
	region_lists->regions_per_chrom = new vector<t_annot_region*>*[(int)chr_ids->size() + 2];

	for(int i_chr = 0; i_chr < (int)chr_ids->size(); i_chr++)
	{
		region_lists->pos_strand_regions_per_chrom[i_chr] = new vector<t_annot_region*>();
		region_lists->neg_strand_regions_per_chrom[i_chr] = new vector<t_annot_region*>();
		region_lists->regions_per_chrom[i_chr] = new vector<t_annot_region*>();
	} // i_chr loop.

	// Go over all the regions and separate them into appropriate list.
	for(int i_reg = 0; i_reg < (int)regions->size(); i_reg++)
	{
		int i_chr = t_string::get_i_str(chr_ids, regions->at(i_reg)->chrom);

		if(i_chr < (int)chr_ids->size())
		{
			region_lists->regions_per_chrom[i_chr]->push_back(regions->at(i_reg));

			if(regions->at(i_reg)->strand == '+')
			{
				region_lists->pos_strand_regions_per_chrom[i_chr]->push_back(regions->at(i_reg));
			}
			else if(regions->at(i_reg)->strand == '-')
			{
				region_lists->neg_strand_regions_per_chrom[i_chr]->push_back(regions->at(i_reg));
			}
			else
			{
				printf("Unknown strand char %c @ %s(%d).\n", regions->at(i_reg)->strand, __FILE__, __LINE__);
				exit(0);
			}
		}
	} // i_reg loop.

	for(int i_chr = 0; i_chr < (int)chr_ids->size(); i_chr++)
	{
		region_lists->pos_strand_regions_per_chrom[i_chr] = new vector<t_annot_region*>();
		region_lists->neg_strand_regions_per_chrom[i_chr] = new vector<t_annot_region*>();
		region_lists->regions_per_chrom[i_chr] = new vector<t_annot_region*>();
	} // i_chr loop.

	// Finally sort the regions.
	return(region_lists);
}

vector<t_annot_region*>* load_BED12(char* bed_fp)
{
	vector<t_annot_region*>* bed_regions = new vector<t_annot_region*>();

	FILE* f_bed = open_f(bed_fp, "r");
	
	while(1)
	{
		char* cur_line = getline(f_bed);
		if(cur_line == NULL)
		{
			break;
		}

		bool skip_line = check_line_skip(cur_line);

		if(!skip_line)
		{
			// Read the mandatory fields.
			int start;
			int end;
			char chrom[100];
			char name[1000];
			int score;
			char strand;
			int thick_start;
			int thick_end;
			char rgb[100];
			int n_exons;
			char block_sizes[100];
			char block_starts[100];
			// chrI    4283    5269    JUNC00000001    90      -       4283    5269    255,0,0 2       75,75   0,911
			if(sscanf(cur_line, "%s %d %d %s %d %c %d %d %s %d %s %s", chrom, &start, &end, name, &score, &strand, &thick_start, &thick_end, rgb, &n_exons, block_sizes, block_starts) != 12)
			{
				printf("Could not read the 12 fields from BED file line:\n%s\n", cur_line);
				exit(0);
			}

			t_annot_region* new_region = new t_annot_region();
			// Translate the start and end of the BED coordinates to the codebase coordinates.
			//new_region->start = translate_coord(start, BED_COORDS::start_base, CODEBASE_COORDS::start_base);
			//new_region->end = translate_coord(end, BED_COORDS::end_base, CODEBASE_COORDS::end_base);
			new_region->start = start;
			new_region->end = end;

			new_region->chrom = t_string::copy_me_str(chrom);
			normalize_chr_id(new_region->chrom);
			new_region->name = t_string::copy_me_str(name);
			new_region->strand = strand;
			new_region->data = NULL;

			// The score is important since it tells how strong this junction is. (Total # of spanning junction reads.)
			new_region->score = score;

			vector<t_annot_region*>* intervals = new vector<t_annot_region*>();

			// Setup all the interval regions.
			int i_block_sizes_str_char = 0;
			int i_block_starts_str_char = 0;
            //fprintf(stderr, "%s\n", cur_line);
			for(int i_ex = 0; i_ex < n_exons; i_ex++)
			{
				char cur_block_size_str[100];
				memset(cur_block_size_str, 0, 100);
				if(!t_string::get_next_token(block_sizes, cur_block_size_str, 100, ",", i_block_sizes_str_char))
				{
					fprintf(stderr, "Could not parse the block sizes string.\n");
					exit(0);
				}

				char cur_block_start_str[100];
				memset(cur_block_start_str, 0, 100);
				if(!t_string::get_next_token(block_starts, cur_block_start_str, 100, ",", i_block_starts_str_char))
				{
					fprintf(stderr, "Could not parse the block starts string.\n");
					exit(0);
				}

				t_annot_region* cur_block = new t_annot_region();
				cur_block->chrom = t_string::copy_me_str(chrom);
				normalize_chr_id(cur_block->chrom);
				cur_block->start = new_region->start + atoi(cur_block_start_str);
				cur_block->end = cur_block->start + atoi(cur_block_size_str);
				cur_block->name = NULL;
				cur_block->data = NULL;
				cur_block->strand = strand;

				//fprintf(stderr, "Block in %s:%d-%d: %d-%d\n", chrom, start, end, cur_block->start, cur_block->end);
				//if(cur_block->start < start || cur_block->end > end)
				//{
				//	fprintf(stderr, "WTF: %s, %s (%s, %s)\n", block_sizes, block_starts, cur_block_start_str, cur_block_size_str);
				//	getc(stdin);
				//}

				intervals->push_back(cur_block);
			} // i_ex loop.
			//getc(stdin);

			new_region->intervals = intervals;
		
			bed_regions->push_back(new_region);
		} // line skip check.

		delete [] cur_line;
	} // file reading loop.

	fclose(f_bed);

	//printf("Loaded %d regions from %s\n", bed_regions->size(), bed_fp);

	return(bed_regions);
}

vector<t_annot_region*>* load_BED(char* bed_fp)
{
	vector<t_annot_region*>* bed_regions = new vector<t_annot_region*>();

	FILE* f_bed = open_f(bed_fp, "r");
	
	while(1)
	{
		char* cur_line = getline(f_bed);
		if(cur_line == NULL)
		{
			break;
		}

		bool skip_line = check_line_skip(cur_line);

		if(!skip_line)
		{
			// Read the mandatory fields.
			int i_cur_char = 0;
			char strand_char = '+';
			char region_name[1000];
			strcpy(region_name, ".");
			int region_score = 0;

			char chrom[100];
			memset(chrom, 0, 100);
			if(!t_string::get_next_token(cur_line, chrom, 100, "\t", i_cur_char))
			{
				fprintf(stderr, "Could not read chromosome in %s\n", cur_line);
				exit(0);
			}
			

			int start = 0;
			char start_str[100];
			memset(start_str, 0, 100);
			if(!t_string::get_next_token(cur_line, start_str, 100, "\t", i_cur_char))
			{
				fprintf(stderr, "Could not read start in %s\n", cur_line);
				exit(0);
			}
			else
			{
				start = atoi(start_str);
			}

			int end = 0;
			char end_str[100];
			memset(end_str, 0, 100);
			if(!t_string::get_next_token(cur_line, end_str, 100, "\t", i_cur_char))
			{
				fprintf(stderr, "Could not read end in %s\n", cur_line);
				exit(0);
			}
			else
			{
				end = atoi(end_str);
			}

			
			// Keep on reading and load until line is finished.
			bool line_finished = false;
			char name_str[1000];
			memset(name_str, 0, 1000);
			if(!t_string::get_next_token(cur_line, name_str, 1000, "\t", i_cur_char))
			{
				// Could not get the next token.
				line_finished = true;
			}
			else
			{
				// Region name is already initialized to '.'.
				strcpy(region_name, name_str);
			}

			if(!line_finished)
			{
				char score_str[1000];
				memset(score_str, 0, 1000);
				if(!t_string::get_next_token(cur_line, score_str, 1000, "\t", i_cur_char))
				{
					line_finished = true;
				}
				else
				{
					//strcpy(region_score, score_str);
					region_score = atoi(score_str);
				}
			}

			if(!line_finished)
			{
				char strand_str[10];
				memset(strand_str, 0, 10);
				if(!t_string::get_next_token(cur_line, strand_str, 10, "\t", i_cur_char))
				{
					line_finished = true;
				}
				else
				{
					strand_char = strand_str[0];
				}
			}

			/*if(sscanf(cur_line, "%s %d %d", chrom, &start, &end) != 3)
			{
				printf("Could not read the mandatory fields from BED file line:\n%s\n", cur_line);
				exit(0);
			}*/

			//char strand = '+'; 
			//if(sscanf(cur_line, "%*s %*s %*s %*s %*s %c", &strand) != 1)
			//{
			//	// Could not read the strand from file, set it to '+' by default.
			//	strand = '+';
			//}

			//int region_score = 0;
			//if(sscanf(cur_line, "%*s %*s %*s %*s %d", &region_score) != 1)
			//{
			//	// Could not read the score from the file.
			//}

			//char region_name[1000];
			//if(sscanf(cur_line, "%*s %*s %*s %s", region_name) != 1)
			//{
			//	// Could not read the strand from file, set it to '+' by default.
			//	strcpy(region_name, ".");
			//}

			t_annot_region* new_region = new t_annot_region();

			// Translate the start and end of the BED coordinates to the codebase coordinates.
			//new_region->start = start - BED_START_BASE + CODEBASE_START_BASE;
			//new_region->end = end - BED_END_BASE + CODEBASE_END_BASE;
			new_region->start = translate_coord(start, BED_COORDS::start_base, CODEBASE_COORDS::start_base);
			new_region->end = translate_coord(end, BED_COORDS::end_base, CODEBASE_COORDS::end_base);
			new_region->chrom = t_string::copy_me_str(chrom);
			normalize_chr_id(new_region->chrom);
			new_region->name = new char[strlen(region_name) + 2];
			strcpy(new_region->name, region_name);
			new_region->strand = strand_char;
			new_region->data = NULL;
			new_region->score = region_score;
			new_region->sort_info = NULL;
		
			bed_regions->push_back(new_region);
		} // line skip check.

		delete [] cur_line;
	} // file reading loop.

	fclose(f_bed);

	//printf("Loaded %d regions from %s\n", bed_regions->size(), bed_fp);

	if(!validate_region_coords(bed_regions))
	{
		fprintf(stderr, "The coordinates are not valid for %s\n", bed_fp);
		exit(0);
	}

	return(bed_regions);
}
/*
Following loads a bed file with all the file information.
*/
vector<t_annot_region*>* load_BED_with_line_information(char* bed_fp)
{
	vector<t_annot_region*>* bed_regions = new vector<t_annot_region*>();

	FILE* f_bed = open_f(bed_fp, "r");
	
	char* cur_line = getline(f_bed);
	while(cur_line != NULL)
	{
		bool skip_line = check_line_skip(cur_line);

		if(!skip_line)
		{
			// Read the mandatory fields.
			int start;
			int end;
			char chrom[100];
			if(sscanf(cur_line, "%s %d %d", chrom, &start, &end) != 3)
			{
				printf("Could not read the mandatory fields from BED file line:\n%s\n", cur_line);
				exit(0);
			}

			// Read the strand information if it is there otherwise set to positive strand.
			char strand = '+';
			char strand_str[10]; 
			if(sscanf(cur_line, "%*s %*d %*d %*s %*s %s", strand_str) != 1)
			{
				// Could not read the strand from file, set it to '+' by default.
				strand = '+';
			} 
			else if(strand_str[0] == '-' || // Verify the read strand information.
				strand_str[0] == '+')
			{
				strand = strand_str[0];
			}

			char* bed_line = new char[strlen(cur_line) + 2];
			strcpy(bed_line, cur_line);

			t_annot_region* new_region = new t_annot_region();
			// Translate the start and end of the BED coordinates to the codebase coordinates.
			new_region->start = translate_coord(start, BED_COORDS::start_base, CODEBASE_COORDS::start_base);
			new_region->end = translate_coord(end, BED_COORDS::end_base, CODEBASE_COORDS::end_base);
			new_region->chrom = t_string::copy_me_str(chrom);
			normalize_chr_id(new_region->chrom);
			new_region->strand = strand;
			new_region->data = (void*)bed_line;
		
			bed_regions->push_back(new_region);
		} // line skip check.

		delete [] cur_line;

		// Read the next line.
		cur_line = getline(f_bed);
	} // file reading loop.

	fclose(f_bed);

	//printf("Loaded %d regions from %s\n", bed_regions->size(), bed_fp);

	return(bed_regions);
}

// Load a narrowPeak file as an annotation file: Necessary for loading and processing ENCODE peak calls.
vector<t_annot_region*>* load_narrowPeak(char* narrowPeak_fp)
{
	printf("Loading narrowPeak file %s\n", narrowPeak_fp);

	vector<t_annot_region*>* np_regions = new vector<t_annot_region*>();

	FILE* f_np = open_f(narrowPeak_fp, "r");
	if(f_np == NULL)
	{
		printf("Could not open narrowPeak file @ %s\n", narrowPeak_fp);
		exit(0);
	}

	char* cur_line = getline(f_np);
	while(cur_line != NULL)
	{
		bool skip_line = check_line_skip(cur_line);

		if(!skip_line)
		{
			int start;
			int end;
			char chrom[100];

			if(sscanf(cur_line, "%s %d %d", chrom, &start, &end) != 3)
			{
				printf("Could not parse narrowPeak file entry line:\n%s\n", cur_line);
				exit(0);
			}

			t_narrowPeak_info* new_np_info = NULL;
			char score_str[100];
			char strand_str[100];
			char signal_val_str[100];
			char pval_str[100];
			char qval_str[100];
			char peak_pos_str[100];
			if(sscanf(cur_line, "%*s %*s %*s %*s %s %s %s %s %s %s",
								score_str,
								strand_str,
								signal_val_str,
								pval_str,
								qval_str,
								peak_pos_str) != 6)
			{
				printf("Could not read narrowPeak entries, not setting custom narrowPeak info.\n");
			}
			else
			{
				new_np_info = new t_narrowPeak_info();
				if(score_str[0] != '.')
				{
					new_np_info->score = atoi(score_str);
				}

				if(strand_str[0] != '.')
				{
					new_np_info->strand = score_str[0];
				}

				if(signal_val_str[0] != '.')
				{
					new_np_info->signal_value = atof(signal_val_str);
				}

				if(pval_str[0] != '.')
				{
					new_np_info->p_value = atof(pval_str);
				}

				if(qval_str[0] != '.')
				{
					new_np_info->q_value = atof(qval_str);
				}

				if(peak_pos_str[0] != '.')
				{
					new_np_info->peak_pos = atoi(peak_pos_str);
					//printf("Setting peak position at %d\n", new_np_info->peak_pos);
				}

				new_np_info->info_type = NARROWPEAK_INFO;
			}

			// narrowPeak files do not contain strand information, everything is on forward strand.
			char strand = '+'; 

			// Allocate and initialize the new region.
			t_annot_region* new_region = new t_annot_region();
			//new_region->start = start - BED_START_BASE + CODEBASE_START_BASE;
			//new_region->end = end - BED_END_BASE + CODEBASE_END_BASE;
			new_region->start = translate_coord(start, BED_COORDS::start_base, CODEBASE_COORDS::start_base);
			new_region->end = translate_coord(end, BED_COORDS::end_base, CODEBASE_COORDS::end_base);
			new_region->chrom = t_string::copy_me_str(chrom);
			normalize_chr_id(new_region->chrom);
			new_region->strand = strand;
			new_region->data = new_np_info;

			// Add the new region to the list.
			np_regions->push_back(new_region);
		}

		delete [] cur_line;
		cur_line = getline(f_np);
	} // narrowPeak file reading loop.

	fclose(f_np);

	printf("Loaded %ld regions from %s\n", np_regions->size(), narrowPeak_fp);
	return(np_regions);
}

double coverage(vector<t_annot_region*>* annot_regions)
{
	double covrg = 0.0;

	for(int i_reg = 0; i_reg < (int)annot_regions->size(); i_reg++)
	{
		if(annot_regions->at(i_reg) != NULL)
		{
			if(annot_regions->at(i_reg)->end < annot_regions->at(i_reg)->start)
			{
				//printf("Region %d has its start index larger than its end index: %d, %d\n", i_reg, 
				//	annot_regions->at(i_reg)->start,
				//	annot_regions->at(i_reg)->end);
			}
			else
			{
				covrg += annot_regions->at(i_reg)->end - annot_regions->at(i_reg)->start + 1;
			}
		}
	} // i_reg loop.

	return(covrg);
}

int get_reg2reg_distance(t_annot_region* reg1, t_annot_region* reg2)
{
	if(reg1->start > reg2->end)
	{
		// Region 1 is to the right of region 2.
		return(reg1->start - reg2->end);
	}
	else if(reg2->start > reg1->end)
	{
		// Region 2 is to the right of region 1.
		return(reg2->start - reg1->end);
	}
	else
	{
		// The regions are overlapping.
		return(0);
	}
}

bool check_line_skip(char* cur_line)
{
	// Skip empty lines.
	if(strlen(cur_line) == 0)
	{
		return(true);
	}

	// Check comment.
	if(cur_line[0] == '#')
	{
		return(true);
	}
	
	// Check track info line.
	char* first_word = new char[strlen(cur_line) + 2];
	strcpy(first_word, cur_line);
	sscanf(cur_line, "%s", first_word);

	if(strcmp(first_word, "track") == 0)
	{
		delete [] first_word;
		return(true);
	}

	delete [] first_word;

	return(false);
}

t_annot_region* duplicate_region(t_annot_region* region_2_dup)
{
	t_annot_region* dup_region = new t_annot_region();

	if(region_2_dup->chrom != NULL)
	{
		dup_region->chrom = new char[strlen(region_2_dup->chrom) + 2];
		strcpy(dup_region->chrom, region_2_dup->chrom);
	}
	else
	{
		dup_region->chrom = NULL;
	}

	if(region_2_dup->name != NULL)
	{
		dup_region->name = new char[strlen(region_2_dup->name) + 2];
		strcpy(dup_region->name, region_2_dup->name);
	}
	else
	{
		dup_region->name = NULL;
	}

	dup_region->start = region_2_dup->start;
	dup_region->end = region_2_dup->end;
	dup_region->strand = region_2_dup->strand;
	dup_region->n_exons = dup_region->n_exons;
	dup_region->score = dup_region->score;
	dup_region->thick_start = dup_region->thick_start;
	dup_region->thick_end = dup_region->thick_end;

	dup_region->intervals = NULL;

	return(dup_region);
}

vector<t_annot_region*>* subtract_annot_regions(t_annot_region* region1, t_annot_region* region2, bool strand_specific)
{
	vector<t_annot_region*>* remaining_regions = new vector<t_annot_region*>();	

	if((!strand_specific && region1->strand != region2->strand) || 
		(strcmp(region1->chrom, region2->chrom) != 0))
	{
		t_annot_region* dup_region = duplicate_region(region1);
		remaining_regions->push_back(dup_region);
		return(remaining_regions);
	}

	// Check if there is overlap between region1 and region2.
	int ol_start = MAX(region1->start, region2->start);
	int ol_end = MIN(region1->end, region2->end);

	if(ol_start < ol_end)
	{
		// There is overlap between regions. Take the region that is excluded from the overlap. 
		// There are 4 possibilities of overlap:
		// os=s1<e1=ol: region2 fully contains region1
		// s1<os<e1=ol: region2 overlaps partialy with region1 on right side.
		// s1=os<ol<e1: region2 overlaps partialy with region1 on left side.
		// s1<os<ol<e1: region1 fully contains region2 with extra parts on the sides.
		if(region1->start == ol_start && 
			region1->end == ol_end)
		{
			// Nothing is left in region1 after exclusion.
		}
		else if(region1->start < ol_start && 
			region1->end == ol_end)
		{
			// Left side of region1 is left.
			t_annot_region* dup_region = duplicate_region(region1);
			dup_region->end = ol_start;
			remaining_regions->push_back(dup_region);
		}
		else if(region1->start == ol_start && 
			ol_end < region1->end)
		{
			t_annot_region* dup_region = duplicate_region(region1);
			dup_region->start = ol_end;
			remaining_regions->push_back(dup_region);
		}
		else if(region1->start < ol_start && 
			ol_end < region1->end)
		{
			t_annot_region* dup_region1 = duplicate_region(region1);
			t_annot_region* dup_region2 = duplicate_region(region1);
			dup_region1->end = ol_start;
			dup_region2->start = ol_end;
			remaining_regions->push_back(dup_region1);
			remaining_regions->push_back(dup_region2);
		}
		else
		{
			fprintf(stderr, "Could not resolve the excluded region with overlapping of: %s:%d-%d and %s:%d-%d.\n", region1->chrom, region1->start, region1->end,
				region2->chrom, region2->start, region2->end);
			exit(0);
		}
	}
	else
	{
		t_annot_region* dup_region = duplicate_region(region1);
		remaining_regions->push_back(dup_region);
	}

	return(remaining_regions);
}

vector<t_annot_region*>* exclude_annot_regions(vector<t_annot_region*>* annot_regions1,
													vector<t_annot_region*>* annot_regions2, 
													bool strand_specific)
{
	if(strand_specific)
	{
		vector<t_annot_region*>* excluded_regions = new vector<t_annot_region*>();

		vector<t_annot_region*>* reg1_pos_regs = get_regions_per_strand(annot_regions1, '+');
		vector<t_annot_region*>* reg1_neg_regs = get_regions_per_strand(annot_regions1, '-');

		vector<t_annot_region*>* reg2_pos_regs = get_regions_per_strand(annot_regions2, '+');
		vector<t_annot_region*>* reg2_neg_regs = get_regions_per_strand(annot_regions2, '-');

		// Do strand specific exclusion.
		vector<t_annot_region*>* reg1_min_reg2_pos = exclude_annot_regions(reg1_pos_regs, reg2_pos_regs);
		vector<t_annot_region*>* reg1_min_reg2_neg = exclude_annot_regions(reg1_neg_regs, reg2_neg_regs);

		// Add the excluded regions.
		excluded_regions->insert(excluded_regions->end(), reg1_min_reg2_pos->begin(), reg1_min_reg2_pos->end());
		excluded_regions->insert(excluded_regions->end(), reg1_min_reg2_neg->begin(), reg1_min_reg2_neg->end());

		return(excluded_regions);
	}
	else
	{
		vector<t_annot_region*>* excluded_regions = exclude_annot_regions(annot_regions1, annot_regions2);
		return(excluded_regions);
	}
}

// Backend function for exclusion.
vector<t_annot_region*>* exclude_annot_regions(vector<t_annot_region*>* annot_regions1,
													vector<t_annot_region*>* annot_regions2)
{
	vector<t_annot_region*>* excluded_regions = new vector<t_annot_region*>();

	vector<t_annot_region*>* merged_regs2 = merge_annot_regions(annot_regions2, 1, false);

	// Restructure each list of regions.
	t_restr_annot_region_list* restructured_regs1 = restructure_annot_regions(annot_regions1);
	t_restr_annot_region_list* restructured_regs2 = restructure_annot_regions(merged_regs2);

	for(int i_chr1 = 0; i_chr1 < (int)restructured_regs1->chr_ids->size(); i_chr1++)
	{
		int i_chr2 = t_string::get_i_str(restructured_regs2->chr_ids, restructured_regs1->chr_ids->at(i_chr1));

		vector<t_annot_region*>* cur_chr_regs1 = restructured_regs1->regions_per_chrom[i_chr1];
		if(i_chr2 < (int)restructured_regs2->chr_ids->size())
		{			
			vector<t_annot_region*>* cur_chr_regs2 = restructured_regs2->regions_per_chrom[i_chr2];

			for(int i_reg1 = 0; i_reg1 < (int)cur_chr_regs1->size(); i_reg1++)
			{
				int i_leftmost_reg = locate_posn_region_per_region_starts(cur_chr_regs1->at(i_reg1)->start, cur_chr_regs2, 0, (int)cur_chr_regs2->size()-1);

				while(i_leftmost_reg > 0 && 
					cur_chr_regs2->at(i_leftmost_reg)->end >= cur_chr_regs1->at(i_reg1)->start)
				{
					i_leftmost_reg--;
				} // i_leftmost_reg loop.

				// Check if there is any overlap while we are not strictly passed over the region.
				bool region_overlaps = false;
				while(!region_overlaps && 
					i_leftmost_reg < (int)cur_chr_regs2->size() &&
					cur_chr_regs2->at(i_leftmost_reg)->start <= cur_chr_regs1->at(i_reg1)->end)
				{
					int ol_start = MAX(cur_chr_regs2->at(i_leftmost_reg)->start, cur_chr_regs1->at(i_reg1)->start);
					int ol_end = MIN(cur_chr_regs2->at(i_leftmost_reg)->end, cur_chr_regs1->at(i_reg1)->end);

					if(ol_end >= ol_start)
					{
						// There is overlap.
						region_overlaps = true;
					}
					i_leftmost_reg++;
				}

				if(!region_overlaps)
				{
					t_annot_region* new_excluded_region = duplicate_region(cur_chr_regs1->at(i_reg1));
					new_excluded_region->data = cur_chr_regs1->at(i_reg1);

					excluded_regions->push_back(new_excluded_region);
				}
			} // i_reg1 loop.
		} // i_chr2 check.
		else
		{
			// chr2 does not exist in region2 list, all the regions in this chromosome for region1 list are excluded from regions2.
			for(int i_reg1 = 0; i_reg1 < (int)cur_chr_regs1->size(); i_reg1++)
			{
				t_annot_region* new_excluded_region = duplicate_region(cur_chr_regs1->at(i_reg1));
				new_excluded_region->data = cur_chr_regs1->at(i_reg1);

				excluded_regions->push_back(new_excluded_region);
			} // i_reg1 loop.
		}
	} // i_chr1 loop.

	// Free memory for merged regions.
	delete_annot_regions(merged_regs2);

	delete_restructured_annot_regions(restructured_regs1);
	delete_restructured_annot_regions(restructured_regs2);

	return(excluded_regions);
}

vector<t_annot_region*>* intersect_annot_regions(vector<t_annot_region*>* annot_regions1,
														vector<t_annot_region*>* annot_regions2,
														bool match_strands,
														bool find_all_overlaps)
{
	// Merged annot. regions.
	if(match_strands)
	{
		vector<t_annot_region*>* plus_strand_regions1 = get_regions_per_strand(annot_regions1, '+');
		vector<t_annot_region*>* plus_strand_regions2 = get_regions_per_strand(annot_regions2, '+');
		vector<t_annot_region*>* intersected_pos_strand_regions = intersect_annot_regions(plus_strand_regions1, plus_strand_regions2, find_all_overlaps);

		// Merge negative strand regions.
		vector<t_annot_region*>* neg_strand_regions1 = get_regions_per_strand(annot_regions1, '-');
		vector<t_annot_region*>* neg_strand_regions2 = get_regions_per_strand(annot_regions2, '-');
		vector<t_annot_region*>* intersected_neg_strand_regions = intersect_annot_regions(neg_strand_regions1, neg_strand_regions2, find_all_overlaps);

		// Pool the merged plus and minus strand regions.
		vector<t_annot_region*>* intersected_annot_regions = new vector<t_annot_region*>();
		intersected_annot_regions->insert(intersected_annot_regions->end(), intersected_pos_strand_regions->begin(), intersected_pos_strand_regions->end());
		intersected_annot_regions->insert(intersected_annot_regions->end(), intersected_neg_strand_regions->begin(), intersected_neg_strand_regions->end());


		delete(plus_strand_regions1);
		delete(plus_strand_regions2);

		delete(neg_strand_regions1);
		delete(neg_strand_regions2);
		return(intersected_annot_regions);
	}
	else
	{
		vector<t_annot_region*>* intersected_annot_regions = intersect_annot_regions(annot_regions1, annot_regions2, find_all_overlaps);
		return(intersected_annot_regions);
	}
}

vector<t_annot_region*>* intersect_annot_regions(vector<t_annot_region*>* orig_annot_regions1,
														vector<t_annot_region*>* orig_annot_regions2,
														bool find_all_overlaps)
{
	vector<t_annot_region*>* intersected_regions = new vector<t_annot_region*>();

	// Copy regions and to data; this ensures that we do not sort original annot regions list 2.
	vector<t_annot_region*>* annot_regions1 = new vector<t_annot_region*>();
	for (int i_reg1 = 0; i_reg1 < (int)orig_annot_regions1->size(); i_reg1++)
	{
		t_annot_region* copy_reg1 = duplicate_region(orig_annot_regions1->at(i_reg1));

		// Set the data of copy reg2 to original region.
		copy_reg1->data = orig_annot_regions1->at(i_reg1);

		annot_regions1->push_back(copy_reg1);
	} // i_reg2 loop.

	// Copy regions and to data; this ensures that we do not sort original annot regions list 2.
	vector<t_annot_region*>* annot_regions2 = new vector<t_annot_region*>();
	for (int i_reg2 = 0; i_reg2 < (int)orig_annot_regions2->size(); i_reg2++)
	{
		t_annot_region* copy_reg2 = duplicate_region(orig_annot_regions2->at(i_reg2));

		// Set the data of copy reg2 to original region.
		copy_reg2->data = orig_annot_regions2->at(i_reg2);

		annot_regions2->push_back(copy_reg2);
	} // i_reg2 loop.

	// Do not merge the regions any more.
	sort_set_sorting_info(annot_regions2, sort_regions); 

	// Restructure each list of regions.
	t_restr_annot_region_list* restructured_regs1 = restructure_annot_regions(annot_regions1);
	t_restr_annot_region_list* restructured_regs2 = restructure_annot_regions(annot_regions2);

	for(int i_chr1 = 0; i_chr1 < (int)restructured_regs1->chr_ids->size(); i_chr1++)
	{
		int i_chr2 = t_string::get_i_str(restructured_regs2->chr_ids, restructured_regs1->chr_ids->at(i_chr1));

		if(i_chr2 < (int)restructured_regs2->chr_ids->size())
		{
			vector<t_annot_region*>* cur_chr_regs1 = restructured_regs1->regions_per_chrom[i_chr1];
			vector<t_annot_region*>* cur_chr_regs2 = restructured_regs2->regions_per_chrom[i_chr2];

			for(int i_reg1 = 0; i_reg1 < (int)cur_chr_regs1->size(); i_reg1++)
			{
				int i_leftmost_reg = locate_posn_region_per_region_starts(cur_chr_regs1->at(i_reg1)->start, cur_chr_regs2, 0, (int)cur_chr_regs2->size()-1);

				// Go back till the cumulative end for the reg2 is to the left of reg1.
				while(i_leftmost_reg > 0 && 
					cur_chr_regs2->at(i_leftmost_reg)->sort_info->cumulative_sorted_end > cur_chr_regs1->at(i_reg1)->start)
				{
					i_leftmost_reg--;
				} // i_leftmost_reg loop.

				// Check if there is any overlap. Check while we are not strictly passed over the region.
				//bool region_overlaps = false;
				int ol_start = 0;
				int ol_end = 0;
				while(i_leftmost_reg < (int)cur_chr_regs2->size() &&
					cur_chr_regs2->at(i_leftmost_reg)->start <= cur_chr_regs1->at(i_reg1)->end)
				{
					ol_start = MAX(cur_chr_regs2->at(i_leftmost_reg)->start, cur_chr_regs1->at(i_reg1)->start);
					ol_end = MIN(cur_chr_regs2->at(i_leftmost_reg)->end, cur_chr_regs1->at(i_reg1)->end);

					if(ol_end >= ol_start)
					{
						// There is overlap: Set the start and end to ol_start and ol_end.
						t_annot_region* new_intersecting_region = duplicate_region(cur_chr_regs1->at(i_reg1));
						new_intersecting_region->start = ol_start;
						new_intersecting_region->end = ol_end;

						// Update the intersection info.
						t_intersect_info* cur_intersection_info = new t_intersect_info();
						cur_intersection_info->src_reg = (t_annot_region*)(cur_chr_regs1->at(i_reg1)->data);
						cur_intersection_info->dest_reg = (t_annot_region*)(cur_chr_regs2->at(i_leftmost_reg)->data);
						cur_intersection_info->l_overlap = ol_end - ol_start + 1;
						new_intersecting_region->data = cur_intersection_info;

						intersected_regions->push_back(new_intersecting_region);

						// Found the first overlap, if looking for just one overlap, return one overlap, otherwise continue since there can be more overlaps.
						if(!find_all_overlaps)
						{
							break;
						}
					}
					i_leftmost_reg++;
				}

				//// Does this region overlap?
				//if(region_overlaps)
				//{
				//	t_annot_region* new_intersecting_region = duplicate_region(cur_chr_regs1->at(i_reg1));
				//	new_intersecting_region->start = ol_start;
				//	new_intersecting_region->end = ol_end;

				//	// Update the intersection info.
				//	t_intersect_info* cur_intersection_info = new t_intersect_info();
				//	cur_intersection_info->src_reg = cur_chr_regs1->at(i_reg1);
				//	cur_intersection_info->dest_reg = cur_chr_regs2->at(i_leftmost_reg);
				//	new_intersecting_region->data = cur_intersection_info;

				//	intersected_regions->push_back(new_intersecting_region);
				//}
			} // i_reg1 loop.
		} // i_chr2 check.
		else
		{
			// This chromosome does not exist in region2 list, cannot overlap two region lists for this chromosome.
		}
	} // i_chr1 loop.

	// Delete the sorting information for regions2.
	delete_sorting_information(annot_regions2);

	delete_restructured_annot_regions(restructured_regs1);
	delete_restructured_annot_regions(restructured_regs2);

	// Delete the copy regions.
	delete_annot_regions(annot_regions1);
	delete_annot_regions(annot_regions2);

	return(intersected_regions);
}

void delete_intersect_info(vector<t_annot_region*>* regions)
{
	for(int i_reg = 0; i_reg < (int)regions->size(); i_reg++)
	{
		if(regions->at(i_reg)->data != NULL)
		{
			t_intersect_info* intersect_info = (t_intersect_info*)(regions->at(i_reg)->data);
			delete intersect_info;
		}
	} // i_reg loop.
}

// Get all the chromosome ids from annotated regions.
vector<char*>* get_chr_ids(vector<t_annot_region*>* annot_regions)
{
	//printf("/*Determining*/ the chromosome list from region list.\n");
	vector<char*>* chr_ids = new vector<char*>();

	for(int i_reg = 0; i_reg < (int)annot_regions->size(); i_reg++)
	{
		bool is_new_id = true;

		// Check if the the current chromosome id is included in the list of chromosome ids already collected, otherwise add it.
		for(int i_chr = 0; 
			is_new_id && i_chr < (int)chr_ids->size(); 
			i_chr++)
		{
			if(strcmp(chr_ids->at(i_chr), annot_regions->at(i_reg)->chrom) == 0)
			{
				is_new_id = false;
			}
		} // i_chr loop

		if(is_new_id)
		{
			char* new_id = new char[strlen(annot_regions->at(i_reg)->chrom) + 2];
			strcpy(new_id, annot_regions->at(i_reg)->chrom);
			chr_ids->push_back(new_id); // Add the new id.
		}
	} // i_reg loop.

	//printf("Determined %d chromosomes from region list.\n", chr_ids->size());
	return(chr_ids);
}

vector<t_annot_region*>* get_regions_per_strand(vector<t_annot_region*>* annot_regions,
													char strand)
{
	vector<t_annot_region*>* chr_regions = new vector<t_annot_region*>();

	for(int i_reg = 0; i_reg < (int)annot_regions->size(); i_reg++)
	{
		//bool new_id = false;

		// Check if the the current chromosome id is included in the list of chromosome ids already collected, otherwise add it.
		if(annot_regions->at(i_reg)->strand == strand)
		{
			// Add the current region to the chromosome regions.
			chr_regions->push_back(annot_regions->at(i_reg));
		}
	} // i_reg loop.

	return(chr_regions);
}

void load_chromosome_lengths_per_tabbed_file(char* chr_lengths_fp, vector<char*>* chr_ids, vector<int>* chr_lengths)
{
	FILE* f_l_chrs = open_f(chr_lengths_fp, "r");

	while(1)
	{
		char* cur_line = getline(f_l_chrs);
		if(cur_line == NULL)
		{
			break;
		}

		char cur_chr_id[1000];
		int cur_l_chr = 0;
		if(sscanf(cur_line, "%s %d", cur_chr_id, &cur_l_chr) != 2)
		{
			fprintf(stderr, "Could not read chromosome lengths from: %s\n", cur_line);
			exit(0);
		} // check if reading is succesful.

		// Normalize the chromosome id before adding it to the list.
		normalize_chr_id(cur_chr_id);
		chr_ids->push_back(t_string::copy_me_str(cur_chr_id));
		chr_lengths->push_back(cur_l_chr);
	} // file reading loop.

	fclose(f_l_chrs);
}

t_annot_region* get_empty_region()
{
	t_annot_region* new_reg = new t_annot_region();
	new_reg->chrom = NULL;
	new_reg->start = 0;
	new_reg->end = 0;
	new_reg->strand = 0;
	new_reg->intervals = 0;
	new_reg->name = 0;
	new_reg->data = 0;
	new_reg->sort_info = NULL;

	return(new_reg);
}

vector<t_annot_region*>* get_regions_per_chromosome(vector<t_annot_region*>* annot_regions,
													char* chr_id)
{
	vector<t_annot_region*>* chr_regions = new vector<t_annot_region*>();

	for(int i_reg = 0; i_reg < (int)annot_regions->size(); i_reg++)
	{
		//bool new_id = false;

		// Check if the the current chromosome id is included in the list of chromosome ids already collected, otherwise add it.
		if(strcmp(annot_regions->at(i_reg)->chrom, chr_id) == 0)
		{
			// Add the current region to the chromosome regions.
			chr_regions->push_back(annot_regions->at(i_reg));
		}
	} // i_reg loop.

	return(chr_regions);
}

vector<t_annot_region*>* subtract_annot_regions(vector<t_annot_region*>* regions1, vector<t_annot_region*>* regions2)
{
	if(regions1 == NULL)
	{
		return(NULL);
	}
	else if(regions2 == NULL)
	{
		vector<t_annot_region*>* subtracted_regions1 = new vector<t_annot_region*>();
		for(int i_reg = 0; i_reg < (int)regions1->size(); i_reg++)
		{
			subtracted_regions1->push_back(duplicate_region(regions1->at(i_reg)));
		} // i_reg loop.

		return(subtracted_regions1);
	}

	vector<t_annot_region*>* subtracted_regions1 = new vector<t_annot_region*>();

	vector<t_annot_region*>* merged_regions2 = merge_annot_regions(regions2, 0);

	// Restructure the lists.
	t_restr_annot_region_list* restructured_regions1 = restructure_annot_regions(regions1);
	t_restr_annot_region_list* restructured_regions2 = restructure_annot_regions(merged_regions2);

	for(int i_chr1 = 0; i_chr1 < (int)restructured_regions1->chr_ids->size(); i_chr1++)
	{
		vector<t_annot_region*>* cur_chr_regs1 = restructured_regions1->regions_per_chrom[i_chr1];

		int i_chr2 = t_string::get_i_str(restructured_regions2->chr_ids, restructured_regions1->chr_ids->at(i_chr1));
		
		// Check if this chromosome exists in the second list
		if(i_chr2 < (int)restructured_regions2->chr_ids->size())
		{
			vector<t_annot_region*>* cur_chr_regs2 = restructured_regions2->regions_per_chrom[i_chr2];

			for(int i_reg1 = 0; i_reg1 < (int)cur_chr_regs1->size(); i_reg1++)
			{
				t_annot_region* cur_rightmost_remaining_region = duplicate_region(cur_chr_regs1->at(i_reg1));

				int cur_left_i = locate_posn_region_per_region_starts(cur_rightmost_remaining_region->start, cur_chr_regs2, 0, (int)cur_chr_regs2->size() - 1);

				while(cur_left_i > 0 &&
					cur_chr_regs2->at(cur_left_i)->end > cur_chr_regs1->at(i_reg1)->start)
				{
					cur_left_i--;
				} // cur_left_i loop.

				// Move over the region 1 and get rid of pieces.
				while(cur_left_i < (int)cur_chr_regs2->size() && 
					cur_chr_regs2->at(cur_left_i)->start <= cur_chr_regs1->at(i_reg1)->end &&
					cur_rightmost_remaining_region != NULL)
				{
					int ol_start = MAX(cur_rightmost_remaining_region->start, cur_chr_regs2->at(cur_left_i)->start);
					int ol_end = MIN(cur_rightmost_remaining_region->end, cur_chr_regs2->at(cur_left_i)->end);

					if(ol_start <= ol_end)
					{
						// Region2 overlaps with region1. Find the remaining regions based on the overlap.

						// Add the remaining regions: These are the parts that are outside the 
						if(cur_rightmost_remaining_region->start < ol_start)
						{
							t_annot_region* new_remaining_region = get_empty_region();
							new_remaining_region->chrom = t_string::copy_me_str(cur_rightmost_remaining_region->chrom);
							new_remaining_region->start = cur_rightmost_remaining_region->start;
							new_remaining_region->end = ol_start-1;
							new_remaining_region->strand = cur_rightmost_remaining_region->strand;
							new_remaining_region->data = cur_chr_regs1->at(i_reg1);

							// Add the left side remaining part to the list.
							subtracted_regions1->push_back(new_remaining_region);
						}

						// Check the right side remaining region.
						if(cur_rightmost_remaining_region->end > ol_end)
						{
							t_annot_region* new_remaining_region = get_empty_region();
							new_remaining_region->chrom = t_string::copy_me_str(cur_rightmost_remaining_region->chrom);
							new_remaining_region->start = ol_end+1;
							new_remaining_region->end = cur_rightmost_remaining_region->end;
							new_remaining_region->strand = cur_rightmost_remaining_region->strand;
							new_remaining_region->data = cur_chr_regs1->at(i_reg1);

							// Free the memory for rightmost remaining region, it is going to be updated.
							delete_annot_regions(cur_rightmost_remaining_region);

							// Set the remaining region to the current remaining region.
							cur_rightmost_remaining_region = new_remaining_region;
						}
						else
						{
							// Nothing is left on the right side: There is overlap and the overlap's end extends beyond end of the remaining region.
							delete_annot_regions(cur_rightmost_remaining_region);
							cur_rightmost_remaining_region = NULL;
						}
					} // overlap check.

					cur_left_i++;
				} // cur_left_i loop.

				// Add the final remaining region.
				if(cur_rightmost_remaining_region != NULL)
				{
					t_annot_region* final_remaining_region = cur_rightmost_remaining_region;

					// Sets the data to point to the original regions where the remnant is coming from.
					final_remaining_region->data = cur_chr_regs1->at(i_reg1);
					subtracted_regions1->push_back(final_remaining_region);
				}
			} // i_reg1 loop.
		}
		else
		{
			for(int i_reg1 = 0; i_reg1 < (int)cur_chr_regs1->size(); i_reg1++)
			{
				t_annot_region* new_remaining_region = duplicate_region(cur_chr_regs1->at(i_reg1));
				subtracted_regions1->push_back(new_remaining_region);
			} // i_reg1 loop.
		} // i_chr2 check.
	} // i_chr1 loop.

	delete_restructured_annot_regions(restructured_regions1);
	delete_restructured_annot_regions(restructured_regions2);
	delete_annot_regions(merged_regions2);
	return(subtracted_regions1);
}

vector<t_annot_region*>* merge_annot_regions(vector<t_annot_region*>* total_annot_regions,
											int max_gap, 
											bool match_strands) // Maximum distance between two annotated regions that are in the merged region.
{
	//printf("Merging %d regions.\n", total_annot_regions->size());

	//fprintf(stderr, "All region list contains %d regions.\n", total_annot_regions->size());

	vector<t_annot_region*>* merged_annot_regions = NULL;

	// Merged annot. regions.
	if(match_strands)
	{
		//printf("Merging with strand information.");
		// Merge plus strand regions.
		vector<t_annot_region*>* plus_strand_regions = get_regions_per_strand(total_annot_regions,
																			'+');
		//printf("%d plus strand regions.\n", plus_strand_regions->size());
		vector<t_annot_region*>* merged_plus_strand_regions = merge_annot_regions(plus_strand_regions, max_gap);

		// Merge negative strand regions.
		vector<t_annot_region*>* neg_strand_regions = get_regions_per_strand(total_annot_regions,
																			'-');
		//printf("%d negative strand regions.\n", neg_strand_regions->size());
		vector<t_annot_region*>* merged_neg_strand_regions = merge_annot_regions(neg_strand_regions, max_gap);

		// Pool the merged plus and minus strand regions.
		merged_annot_regions = new vector<t_annot_region*>();
		for(int i_reg = 0; i_reg < (int)merged_plus_strand_regions->size(); i_reg++)
		{
			merged_annot_regions->push_back(merged_plus_strand_regions->at(i_reg));
		} // i_reg loop

		for(int i_reg = 0; i_reg < (int)merged_neg_strand_regions->size(); i_reg++)
		{
			merged_annot_regions->push_back(merged_neg_strand_regions->at(i_reg));
		} // i_reg loop

		merged_plus_strand_regions->clear();
		delete(merged_plus_strand_regions);

		merged_neg_strand_regions->clear();
		delete(merged_neg_strand_regions);
	}
	else
	{
		 merged_annot_regions = merge_annot_regions(total_annot_regions, max_gap);
	}

	//printf("Merged into %d regions\n", merged_annot_regions->size());
	return(merged_annot_regions);
}

// Merge the regions in the regions list. This is the backend function for all the merging functions.
// Merges the regions irrespective of the chromosome id list. Build the list on the fly.
vector<t_annot_region*>* merge_annot_regions(vector<t_annot_region*>* total_annot_regions,
											int max_gap) // Maximum distance between two annotated regions that are in the merged region.
{
	//printf("Merging %d regions.\n", total_annot_regions->size());

	// Determine the chromosome ids from the total region list.
	vector<char*>* chr_ids = get_chr_ids(total_annot_regions);
	//printf("Found %d different chromosomes in regions list.\n", chr_ids->size());

	//fprintf(stderr, "All region list contains %d regions.\n", total_annot_regions->size());

	// Merged annot. regions.
	vector<t_annot_region*>* merged_annot_regions = new vector<t_annot_region*>();

	// Go over all the regions in total_annot_regions and merge necessary ones.
	for(int i_chr = 0; i_chr < (int)chr_ids->size(); i_chr++)
	{
		vector<t_annot_region*>* cur_chr_regions = get_regions_per_chromosome(total_annot_regions,
																				chr_ids->at(i_chr));

        sort(cur_chr_regions->begin(), cur_chr_regions->end(), sort_regions);
			
		if(cur_chr_regions->size() > 0)
		{
			//printf("Loaded %d regions in %s\n", cur_chr_regions->size(), chr_ids->at(i_chr));

			int i = 0;
			int cur_start = cur_chr_regions->at(i)->start;
			int cur_end = cur_chr_regions->at(i)->end;

			// Find the next read that is to be merged in this chromosome.
			while(i < (int)cur_chr_regions->size())
			{
				// Is this not the last region in the region list?
				if(i+1 < (int)cur_chr_regions->size())
				{
					// Check max_gap condition for merging: Start of the next region should be at most max_gap nucs away from end of current region.
					if(cur_chr_regions->at(i+1)->start <= (cur_end + max_gap))
					{
						//fprintf(stderr, "Merging: <%d-%d>, cur_end = %d\n", cur_chr_regions->at(i+1)->start, cur_chr_regions->at(i+1)->end, cur_end);
						// Update cur_end, if the end of next region is passing cur_end, update cur_end, otherwise this region is a subset of previous regions.
						cur_end = (cur_end < cur_chr_regions->at(i+1)->end)?(cur_chr_regions->at(i+1)->end):cur_end;
					}
					else
					{
						// Add the region that has accumulated so far: Note that the last region is processed in the previous iteration, it therefore is not 
						// necessary to process that last region any more.
						t_annot_region* new_region = new t_annot_region();
						new_region->start = cur_start;
						new_region->end = cur_end;
						new_region->chrom = new char[strlen(cur_chr_regions->at(i)->chrom) + 2];
						strcpy(new_region->chrom, cur_chr_regions->at(i)->chrom);
						new_region->strand = cur_chr_regions->at(i)->strand;

						merged_annot_regions->push_back(new_region);
						//fprintf(stderr, "Adding merged region <%d-%d>.\n", merged_annot_regions->back()->start, merged_annot_regions->back()->end);

						// Set the current start to start of next region.
						cur_start = cur_chr_regions->at(i+1)->start;
						cur_end = cur_chr_regions->at(i+1)->end;
						//printf("Initiated a new region @ <%d-.>\n", cur_start);
					}
				}
				else
				{
					// This is the last region. Set the max to its ending and add the last merged region.
					t_annot_region* new_region = new t_annot_region();
					new_region->start = cur_start;
					//new_region->end = cur_chr_regions->at(i)->end;
					new_region->end = cur_end;
					new_region->chrom = new char[strlen(cur_chr_regions->at(i)->chrom) + 2];
					strcpy(new_region->chrom, cur_chr_regions->at(i)->chrom);
					new_region->strand = cur_chr_regions->at(i)->strand;

					merged_annot_regions->push_back(new_region);

					//fprintf(stderr, "Adding merged region <%d-%d>.\n", merged_annot_regions->back()->start, merged_annot_regions->back()->end);
					break;
				}

				i++;
			} // i loop 
		} // region size check to make sure that there are regions in this chromosome.

		// Delete the regions for current chromosome. And move to the next one.
		cur_chr_regions->clear();
		delete(cur_chr_regions);
	} // chr check.

	// Clean the chromosome ids.
	t_string::clean_string_list(chr_ids);

	//printf("Merged into %d regions\n", merged_annot_regions->size());
	return(merged_annot_regions);
}

/*
The chromosome and strand matching are not checked, yet.
*/
vector<t_annot_region*>* merge_annot_regions(vector<t_annot_region*>* annot_regions1,
											vector<t_annot_region*>* annot_regions2,
											int max_gap, 
											bool match_strands) // Maximum distance between two annotated regions that are in the merged region.
{
	fprintf(stderr, "Merging %ld versus %ld regions.\n", annot_regions1->size(), annot_regions2->size());

	vector<t_annot_region*>* total_annot_regions = new vector<t_annot_region*>();
	for(int i = 0; i < (int)annot_regions1->size(); i++)
	{
		total_annot_regions->push_back(annot_regions1->at(i));
	} // i loop.

	for(int i = 0; i < (int)annot_regions2->size(); i++)
	{
		total_annot_regions->push_back(annot_regions2->at(i));
	} // i loop.

	vector<t_annot_region*>* merged_annot_regions = merge_annot_regions(total_annot_regions,
																			max_gap, 
																			match_strands);

	delete(total_annot_regions);

	return(merged_annot_regions);
}

vector<t_annot_region*>* merge_annot_regions(vector<vector<t_annot_region*>*>* annot_regions_list,
												int max_gap, 
												bool match_strands)
{
	vector<t_annot_region*>* total_annot_regions = new vector<t_annot_region*>();
	for(int i_reg_list = 0; i_reg_list < (int)annot_regions_list->size(); i_reg_list++)
	{
		vector<t_annot_region*>* cur_annot_regions = annot_regions_list->at(i_reg_list);
		for(int i = 0; i < (int)cur_annot_regions->size(); i++)
		{
			total_annot_regions->push_back(cur_annot_regions->at(i));
		} // i loop.
	} // i_reg_list loop.

	vector<t_annot_region*>* merged_annot_regions = merge_annot_regions(total_annot_regions,
																			max_gap, 
																			match_strands);

	delete(total_annot_regions);

	return(merged_annot_regions);
}

vector<t_annot_region*>* sort_regions_per_chromosome_per_strand(vector<t_annot_region*>* annot_region_list)
{
	vector<t_annot_region*>* sorted_region_list = new vector<t_annot_region*>();

	// No mallocation in following.
	vector<t_annot_region*>* pos_strand_regions = get_regions_per_strand(annot_region_list, '+');

	vector<char*>* chr_ids = get_chr_ids(annot_region_list);

	for(int i_chr = 0; i_chr < (int)chr_ids->size(); i_chr++)
	{
		vector<t_annot_region*>* cur_chr_regions = get_regions_per_chromosome(pos_strand_regions, chr_ids->at(i_chr));

		sort(cur_chr_regions->begin(), cur_chr_regions->end(), sort_regions);

		for(int i_reg = 0; i_reg < (int)cur_chr_regions->size(); i_reg++)
		{
			sorted_region_list->push_back(cur_chr_regions->at(i_reg));
		}
	} // i_chr loop.
	delete(pos_strand_regions);

	vector<t_annot_region*>* neg_strand_regions = get_regions_per_strand(annot_region_list, '-');
	for(int i_chr = 0; i_chr < (int)chr_ids->size(); i_chr++)
	{
		vector<t_annot_region*>* cur_chr_regions = get_regions_per_chromosome(neg_strand_regions, chr_ids->at(i_chr));

		sort(cur_chr_regions->begin(), cur_chr_regions->end(), sort_regions);

		for(int i_reg = 0; i_reg < (int)cur_chr_regions->size(); i_reg++)
		{
			sorted_region_list->push_back(cur_chr_regions->at(i_reg));
		}
	} // i_chr loop.
	delete(neg_strand_regions);

	// Delete chr_ids.
	for(int i_chr = 0; i_chr < (int)chr_ids->size(); i_chr++)
	{
		delete [] chr_ids->at(i_chr);
	}
	delete(chr_ids);

	return(sorted_region_list);
}

bool sort_regions_per_increasing_length(t_annot_region* region1, t_annot_region* region2)
{
	return((region1->end - region1->start) < (region2->end - region2->start));
}

bool sort_regions_per_decreasing_length(t_annot_region* region1, t_annot_region* region2)
{
	return((region1->end - region1->start) > (region2->end - region2->start));
}

bool sort_regions_per_dbl_score_decreasing(t_annot_region* region1, t_annot_region* region2)
{
	return(region1->dbl_score < region2->dbl_score);
}

bool sort_regions_per_dbl_score(t_annot_region* region1, t_annot_region* region2)
{
	return(region1->dbl_score < region2->dbl_score);
}

bool sort_regions_per_score(t_annot_region* region1, t_annot_region* region2)
{
	return(region1->score < region2->score);
}

bool sort_regions_per_ends(t_annot_region* region1, t_annot_region* region2)
{
	return(region1->end < region2->end);
}

// Necessary for fast comparison of the regions.
bool sort_regions(t_annot_region* region1, t_annot_region* region2)
{
	if(region1->start < region2->start)
	{
		return(true);
	}
	else if(region1->start == region2->start)
	{
		if(region1->end < region2->end)
		{
			return(true);
		}
		else
		{
			return(false);
		}
	}
	else
	{
		return(false);
	}
}

void delete_annot_regions(t_annot_region* region)
{
	if(region != NULL)
	{
		delete [] region->chrom;

		if(region->name != NULL)
		{
			delete [] region->name;
		}

		delete(region);
	}
}

/*
Clean a list of annotated regions.
*/
void delete_annot_regions(vector<t_annot_region*>* region_list)
{
	for(int i_reg = 0; i_reg < (int)region_list->size(); i_reg++)
	{
		if(region_list->at(i_reg) != NULL)
		{
			delete [] region_list->at(i_reg)->chrom;

			if(region_list->at(i_reg)->name != NULL)
			{
				delete [] region_list->at(i_reg)->name;
			}

			delete(region_list->at(i_reg));
		}
	} // i_reg loop.

	region_list->clear();
	delete(region_list);
}

void delete_annot_regions_with_line_information(vector<t_annot_region*>* region_list)
{
	for(int i_reg = 0; i_reg < (int)region_list->size(); i_reg++)
	{
		if(region_list->at(i_reg) != NULL)
		{
			delete [] region_list->at(i_reg)->chrom;
			char* cur_line_info = (char*)region_list->at(i_reg)->data;
			if(cur_line_info != NULL)
			{
				delete [] cur_line_info;
			}
			delete(region_list->at(i_reg));
		}
	} // i_reg loop.

	region_list->clear();
	delete(region_list);
}

int region_5p_accessor(void* void_ptr)
{
	t_annot_region* reg_ptr = (t_annot_region*)void_ptr;
	return(reg_ptr->start);
}

int region_3p_accessor(void* void_ptr)
{
	t_annot_region* reg_ptr = (t_annot_region*)void_ptr;
	return(reg_ptr->end);
}

int locate_posn_region_per_region_starts(int start_posn, vector<t_annot_region*>* region_list, int i, int j)
{
	int i_mid = (i + j) / 2;

	if(i_mid == i ||
		i_mid == j)
	{
		//return(i);
		while(i_mid > 0 && region_list->at(i_mid)->start >= start_posn)
		{
			i_mid--;
		}

		return(i_mid);
	}

	if(region_list->at(i_mid)->start == start_posn)
	{
		while(i_mid > 0 && region_list->at(i_mid)->start >= start_posn)
		{
			i_mid--;
		}

		return(i_mid);
	}
	else if(region_list->at(i_mid)->start > start_posn)
	{
		return(locate_posn_region_per_region_starts(start_posn, region_list, i, i_mid));
	}
	else if(region_list->at(i_mid)->start < start_posn)
	{
		return(locate_posn_region_per_region_starts(start_posn, region_list, i_mid, j));
	}
	else
	{
		fprintf(stderr, "Current indices are (%d, %d), searching for %d @ %s(%d)\n", region_list->at(i)->start, region_list->at(j)->start,
			start_posn, __FILE__, __LINE__);
		exit(0);
	}
}

int locate_posn_per_sorted_posn_list(int posn, vector<int>* sorted_posn_list, int i, int j)
{
	int i_mid = (i + j) / 2;

	if(i_mid == i ||
		i_mid == j)
	{
		return(i);
	}

	if(sorted_posn_list->at(i_mid) == posn)
	{
		// Move back while the entry is larger than or equal to the requestd position.
		while(i_mid > 0 &&
			sorted_posn_list->at(i_mid) >= posn)
		{
			i_mid--;
		}

		//// If the above loop hit the end, move back once.
		//if(i_mid == sorted_posn_list->size())
		//{
		//	i_mid--;
		//}

		//// If the above loop moved beyond the query move back once, which is the exact element just before query.
		//if(i_mid > 0 &&
		//	sorted_posn_list->at(i_mid) > posn)
		//{
		//	i_mid--;

		return(i_mid);
	}
	else if(sorted_posn_list->at(i_mid) > posn)
	{
		return(locate_posn_per_sorted_posn_list(posn, sorted_posn_list, i, i_mid));
	}
	else if(sorted_posn_list->at(i_mid) < posn)
	{
		return(locate_posn_per_sorted_posn_list(posn, sorted_posn_list, i_mid, j));
	}
	else
	{
		fprintf(stderr, "Current indices are (%d, %d), searching for %d @ %s(%d)\n", sorted_posn_list->at(i), sorted_posn_list->at(j),
			posn, __FILE__, __LINE__);
		exit(0);
	}
}

bool validate_region_coords(vector<t_annot_region*>* regions)
{
	for(int i_reg = 0; i_reg < (int)regions->size(); i_reg++)
	{
		if(regions->at(i_reg)->start > regions->at(i_reg)->end)
		{
			fprintf(stderr, "%s:%d-%d is not valid.\n", regions->at(i_reg)->chrom, regions->at(i_reg)->start, regions->at(i_reg)->end);
			return(false);
		}
	} // i_reg loop.

	return(true);
}


