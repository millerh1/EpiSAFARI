<html>
<font face="arial">
<div style="text-align:center"><img src="episafari_logo.png" alt="Could not load logo." width="800" align="center"></div>
<h1>EpiSAFARI</h1>
EpiSAFARI is a command line tool for detection of signal features from the functional genomics signal profiles. <br><br>
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
- Signal Profile in bedGraph format, <br>
- Mapped reads in SAM/bowtie/eland/... format. <br>
</div><br>
and outputs:<br>
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
- The spline smoothed signal profile. <br>
- The valleys, local minima and maxima in the signal profile. <br>
</div>
<br>

The output is formatted as an extended BED file with multiple columns. Please refer below for the specification of output file format.

<h2>Download and Installation</h2>
You can download EpiSAFARI code by clicking on the green "Clone or Download" button and downloading zip or by checking out via git. After that, navigate to the checkout (or download) directory. If you downloaded the zip file of the source, unzip it using "unzip master.zip".<br><br>

You need to have g++, GSL, and zLib libraries installed for building EpiSAFARI. If they are not installed, type:
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
sudo yum -y install gsl gsl-devel gcc-c++ zlib zlib-devel
<br>
</font></i>
</div><br>

This should install the necessary GSL libraries for building EpiSAFARI correctly.<br><br>

Now EpiSAFARI can be built using:<br>
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
make clean<br>
make
</font></i>
</div><br>
The executable is located under directory <font face="courier">bin/</font>. 

It may be useful to install <a href="http://samtools.sourceforge.net/">samtools</a> for processing BAM files.

To get help on which options are available, use:
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
./bin/EpiSAFARI -help
</font></i>
</div>

<h2>Usage</h2>
EpiSAFARI run starts with setting up the input files. (Note that we use samtools for converting BAM file to SAM files.). EpiSAFARI can take bedGraph files and mapped reads
directly as input. It is necessary to divide the data into chromosomes.

<h2>Building input with bedGraph and bigWig files</h2>
We show an example from ENCODE project below:
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
wget -c http://hgdownload.soe.ucsc.edu/goldenPath/hg19/encodeDCC/wgEncodeBroadHistone/wgEncodeBroadHistoneGm12878H3k04me3StdSigV2.bigWig <br>
wget http://hgdownload.soe.ucsc.edu/admin/exe/linux.x86_64/bigWigToBedGraph <br>
chmod 755 bigWigToBedGraph <br>
./bigWigToBedGraph wgEncodeBroadHistoneGm12878H3k04me3StdSigV2.bigWig wgEncodeBroadHistoneGm12878H3k04me3StdSigV2.bigWig.bgr <br>
mkdir bedGraphs <br>
./bin/EpiSAFARI -separate_bedGraph_2_chromosomes wgEncodeBroadHistoneGm12878H3k04me3StdSigV2.bigWig.bgr bedGraphs <br>
</font></i>
</div><br>
If there are multiple replicates to be pooled, they can be done at once or separately. If done separately, EpiSAFARI pools the bedGraphs automatically and uses the total signal profile in the analyses.<br>

<h2>Building input with mapped read files</h2>
EpiSAFARI can also process mapped read files, for example in SAM format. We show again an example from the ENCODE Project: <br>
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
wget -c http://hgdownload.soe.ucsc.edu/goldenPath/hg19/encodeDCC/wgEncodeBroadHistone/wgEncodeBroadHistoneGm12878H3k04me1StdAlnRep1V2.bam <br>
mkdir processed_reads <br>
samtools view wgEncodeBroadHistoneGm12878H3k04me1StdAlnRep1V2.bam | EpiSAFARI -preprocess_reads SAM stdin processed_reads <br>
</font></i>
</div><br>
If there are multiple replicates of reads to be pooled, they can be done at once or separately. If done separately, EpiSAFARI pools the reads automatically and uses the total signal profile in the analyses.<br><br>

We strongly recommend removing duplicates from the reads. This decreases feature identification time quite much:<br>
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
mkdir processed_reads/sorted processed_reads/dedup <br>
./bin/EpiSAFARI -sort_reads processed_reads processed_reads/sorted <br>
./bin/EpiSAFARI -remove_duplicates processed_reads/sorted 2 processed_reads/dedup <br>
</font></i>
</div><br>

<h2>Spline Fitting to the Data</h2>
After input files are setup, we perform spline fitting to the read coverage signals. The raw data generated by EpiSAFARI is rather large so it is useful to gzip them: <br>
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
n_spline_coeffs=10<br>
spline_order=5<br>
max_max_err=5<br>
max_avg_err=3<br>
l_win=1000<br>
sparse_data=0<br>
l_post_filter=-1<br>
<br>
# bedGraph files: <br>
./bin/EpiSAFARI -bspline_encode bedGraphs ${n_spline_coeffs} ${spline_order} ${max_max_err} ${max_avg_err} ${l_win} ${sparse_data} ${l_post_filter}<br>
# mapped read files: <br>
./bin/EpiSAFARI -bspline_encode processed_reads/dedup ${n_spline_coeffs} ${spline_order} ${max_max_err} ${max_avg_err} ${l_win} ${sparse_data} ${l_post_filter}<br>
</font></i>
</div><br>

<i>n_spline_coeffs</i> controls the number of knots that are used to fit b-spline. It should not be set to a very high value as this may cause overfitting of the data.<br>

<h2>Feature Detection</h2>
We next do feature identification. We first download the multi-mappability signal then identify the features:<br>

```
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
mkdir hg19_36bp <br>
cd hg19_36bp <br>
wget -c http://archive.gersteinlab.org/proj/MUSIC/multimap_profiles/hg19/hg19_36bp.tar.bz2 <br>
tar -xvjf hg19_36bp.tar.bz2 <br>
cd .. <br>
mkdir hg19_seq
cd hg19_seq
wget -c http://hgdownload.soe.ucsc.edu/goldenPath/hg19/bigZips/chromFa.tar.gz <br>
tar -xvzf chromFa.tar.gz <br>
./bin/EpiSAFARI -preprocess_FASTA . fa . <br>
cd .. <br>
max_trough_sig=1000 <br>
min_summit_sig=5 <br>
min_summit2trough_frac=1.2 <br>
l_vic=125 <br>
min_multimapp=1.2 <br>
# bedGraph files: <br>
./bin/EpiSAFARI -get_significant_extrema bedGraphs ${max_trough_sig} ${min_summit_sig} ${min_summit2trough_frac} ${l_vic} hg19_36bp ${min_multimapp} hg19_seq<br>
# mapped read files: <br>
./bin/EpiSAFARI -get_significant_extrema processed_reads/dedup ${max_trough_sig} ${min_summit_sig} ${min_summit2trough_frac} ${l_vic} hg19_36bp ${min_multimapp} hg19_seq<br>
</font></i>
</div><br>
```

<h2>Feature Annotation</h2>
We finally perform feature annotation. We first download the GENCODE gene annotation gff file:<br>
```
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
wget -c ftp://ftp.ebi.ac.uk/pub/databases/gencode/Gencode_human/release_19/gencode.v19.annotation.gff3.gz <br>
<br>
l_promoter=1000 <br><br>
# bedGraph files: <br>
./bin/EpiSAFARI -annotate_features bedGraphs gencode.v19.annotation.gff3.gz ${l_promoter} annotated_features.bed <br>
# mapped read files: <br>
./bin/EpiSAFARI -annotate_features processed_reads/dedup gencode.v19.annotation.gff3.gz ${l_promoter} annotated_features.bed <br>
</font></i>
</div><br>
```

Finally, you can also add ENCODE2 transcription factor binding annotations. We have built the GFF file for the uniformly processed peaks of 690 transcription factors from the ENCODE2 cell lines that you can download and use to annotate the features:
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
wget http://harmancilab.org/tools/EpiSAFARI/wgEncodeAwgTfbs.gff.gz <br>
<br>
./bin/EpiSAFARI -annotate_features bedGraphs wgEncodeAwgTfbs.gff.gz 0 annotated_features.bed <br>
</font></i>
</div><br>

<h2>Sparse Mode</h2>
EpiSAFARI can also process sparse signals. Examples of these include DNA methylation data, which happens only at cytosine nucleotides. For this, run EpiSAFARI with sparse_data=1 to enable sparse signal smoothing. We also turn on post-median filter on.<br><br>

We demonstrate this option on a DNA methylation dataset from Roadmap Epigenome Project:
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier" size=1>
wget -c ftp://ftp.ncbi.nlm.nih.gov/pub/geo/DATA/roadmapepigenomics/by_experiment/Bisulfite-Seq/brain_hippocampus_middle/GSM1112838_BI.Brain_Hippocampus_Middle.Bisulfite-Seq.149.wig.gz <br>
<br>
wget http://hgdownload.cse.ucsc.edu/admin/exe/linux.x86_64/bigWigToBedGraph <br>
wget http://hgdownload.cse.ucsc.edu/admin/exe/linux.x86_64/wigToBigWig <br>
wget http://hgdownload.cse.ucsc.edu/admin/exe/linux.x86_64/fetchChromSizes <br>
chmod 755 fetchChromSizes <br>
chmod 755 wigToBigWig <br>
chmod 755 bigWigToBedGraph <br>
<br>
gzip -cd GSM1112838_BI.Brain_Hippocampus_Middle.Bisulfite-Seq.149.wig.gz > GSM1112838_BI.Brain_Hippocampus_Middle.Bisulfite-Seq.149.wig <br>
./fetchChromSizes hg19 > hg19.list <br>
./wigToBigWig GSM1112838_BI.Brain_Hippocampus_Middle.Bisulfite-Seq.149.wig hg19.list GSM1112838_BI.Brain_Hippocampus_Middle.Bisulfite-Seq.149.wig.bw <br>
./bigWigToBedGraph GSM1112838_BI.Brain_Hippocampus_Middle.Bisulfite-Seq.149.wig.bw GSM1112838_BI.Brain_Hippocampus_Middle.Bisulfite-Seq.149.wig.bw.bgr <br>
 <br>
mkdir GSM1112838_bedGraphs <br>
./bin/EpiSAFARI -separate_bedGraph_2_chromosomes GSM1112838_BI.Brain_Hippocampus_Middle.Bisulfite-Seq.149.wig.bw.bgr GSM1112838_bedGraphs <br>
 <br>
n_spline_coeffs=10 <br>
spline_order=5 <br>
max_max_err=0.3 <br>
max_avg_err=0.3 <br>
l_win=5000 <br>
sparse_data=1 <br>
l_post_filter=50 <br>
<br>
./bin/EpiSAFARI -bspline_encode GSM1112838_bedGraphs ${n_spline_coeffs} ${spline_order} ${max_max_err} ${max_avg_err} ${l_win} ${sparse_data} ${l_post_filter} <br>
 <br>
max_trough_sig=1 <br>
min_summit_sig=0.5 <br>
min_summit2trough_frac=1.2 <br>
l_vic=5000 <br>
min_multimapp=1.2 <br>
./bin/EpiSAFARI -get_significant_extrema GSM1112838_bedGraphs ${max_trough_sig} ${min_summit_sig} ${min_summit2trough_frac} ${l_vic} hg19_36bp ${min_multimapp}<br>
</font></i>
</div><br>

<h2>Visualization of the Signal</h2>
IGV can be used to visualize the spline coded signals. These are stored as bedGraphs files under data directory. <br><br>

To visualize the spline smoothed signal profile for chromosome 1, use following:
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
wget http://hgdownload.cse.ucsc.edu/admin/exe/linux.x86_64/bedGraphToBigWig <br>
chmod 755 bedGraphToBigWig <br>
gzip -cd bedGraphs/spline_coded_1.bgr.gz > bedGraphs/spline_coded_1.bgr <br>
sed -i 's/chr//g' hg19.list  <br>
bedGraphToBigWig bedGraphs/spline_coded_1.bgr hg19.list spline_coded_1.bgr.bw
</font></i>
</div><br>

The bigWig file named <i>spline_coded_1.bgr.bw</i> can be opened in IGV to visualize the spline smoothed signal.

<h2>Output format</h2>
EpiSAFARI outputs the identified features to files named "significant_valleys_[chrosome id].bed" for each chromosome.<br><br>

This is an extended bed file with following columns:<br>
<div style="padding:8px;background-color:#ddd;line-height:1.4;">
<i><font face="courier">
<ol>
<li>[Chromosome]: Chromosome ID </li>
<li>[Left maxima position]: Position the left maximum of the valley	</li>
<li>[Right maxima position]: Position the right maximum of the valley </li>
<li>[Minima position]: Position of the minimum of the valley </li>
<li>[Left maxima signal]: Signal at the left maxima	</li>
<li>[Right maxima signal]: Signal at the right maxima </li>
<li>[Minima signal]: Signal at the minima </li>
<li>[Average multi-mappability signal]: Average multi-mappability signal on the valley </li>
<li>[Maximum multi-mappability signal]: MAximum multi-mappability signal on the valley </li>
<li>[Left hill quality]: Fraction of the nucleotides on the left hill</li>
<li>[Right hill quality]: Fraction of the nucleotides on the right hill</li>
<li>[Nucleotide counts]: Count of nucleotides in the valley: A, C, G, T counts from left maximum to right maximum</li>
<li>[Annotation]: Annotated element's name and type of the element (gene, exon, transcript, promoter, TF_peak)</li>
</font></i>
</div><br>

</html>