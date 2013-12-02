#!/usr/bin/env python

"""The script runs a single test executable and compares its output image with 
a set of reference images generated by other magics versions. If number of 
different pixels between the output image and any of the reference images is 
larger than the threshold parameter, the script returns an error value. 

If output directory parameter is provided, a html report is generated for the test.
"""

__author__  = 'cgjd'
__date__    = '2013-09-24'
__version__ = '0.1'

#Python standard library 
import sys
import json
import os
import resource
from subprocess import call,check_output,Popen,PIPE
from optparse import OptionParser
from datetime import datetime
 
#Project modules
from regression_util import extension,prefix,suffix,writeHtmlReport,usage2Dict,ImageMagick_compare,PerceptualDiff_compare,splitOutput,resultLabel

#####################################################################
#####################################################################
#####################################################################

def compare(timestamp,branch_name,versions,interpreter,executable,reference,threshold,output_dir):

    #print input parameters
    def l(t,n): return (t+' '*n)[:n]
    print l('timestamp:',        20), timestamp
    print l('test executable:',  20), (interpreter +' '+ executable).strip()
    print l('test reference:',   20), reference
    print l('threshold:',        20), '%.2f%%'%threshold
    print l('versions:',         20), versions 
    print l('output dir (HTML):',20), output_dir


    #run the test
    p= None
    if not interpreter=='':
        p = Popen([interpreter,executable],stdout=PIPE, stderr=PIPE)
    else:
        p = Popen(executable,stdout=PIPE,stderr=PIPE,shell=True)

    #get test's run information
    stdout,stderr= p.communicate()
    usage= usage2Dict(resource.getrusage(resource.RUSAGE_CHILDREN))

    #check if output generated
    if not os.path.exists(reference):
        sys.stderr.write(u"TEST FAILED: Output file '%s' has not been generated.\n"%reference)
        sys.exit(1)

    #check if output is a PS or PDF file      
    ref_pages= {}
    ref_ver_pages= {}
    if reference[-2:]=='ps' or reference[-2:]=='pdf': 

        #POSSIBLY SEVERAL pages -> split output into one-page images
        ref_pages= splitOutput(reference)
        
        #Split the reference versions into one-page images
        for version in versions:
            try:
                ver_ref= prefix(reference,version+'_')
                ref_ver_pages[version]= splitOutput(ver_ref)
                assert(len(ref_pages)==len(ref_ver_pages[version]))
            except:
                sys.stderr.write(u"TEST FAILED: Output file '%s' has %d pages but reference file '%s' has %d pages. They can not be compared.\n"%(reference,len(ref_pages),ver_ref,len(ref_ver_pages[version])))
                sys.exit(1)
        #print 'ref_ver_pages=',ref_ver_pages

    else:
        #only a single page -> single image
        ref_pages= [reference]
        for version in versions:
            ver_ref= prefix(reference,version+'_')
            ref_ver_pages[version]= [ver_ref]
    #print 'ref_pages=',ref_pages
    #print 'ref_ver_pages=',ref_ver_pages

    #get number of pixels of output images
    pixels= []
    for ref_page in ref_pages:
        description= check_output(['identify',ref_page])                 
        #description="reference PNG 994x1402 994x1402+0+0 8-bit PseudoClass 9c 33.6KB 0.020u 0:00.020"
        x,y= [int(x) for x in description.split(' ')[2].split('x')]
        pixels.append(x*y)

    #for each reference version
    diff,pdiff,result= {},{},{}
    for version in versions:
        if not diff.has_key(version):   diff[version]=   {}
        if not pdiff.has_key(version):  pdiff[version]=  {}
        if not result.has_key(version): result[version]= {}
                
        #for each page and reference page
        for ipage in range(len(ref_pages)):
            ref_page= ref_pages[ipage]
            ref_ver_page= ref_ver_pages[version][ipage]
            
            #compare with test's output and keep number of different pixels
            ver_dif_page= suffix(ref_ver_page,'_diff')
            diff[version][ipage]= ImageMagick_compare(ref_page,ref_ver_page,ver_dif_page)
            ver_pdiff_page= suffix(ref_ver_page,'_pdif')
            pdiff[version][ipage]= PerceptualDiff_compare(ref_page,ref_ver_page,ver_pdiff_page)
            
            #Get the result labels: OK,Check,Warning,Error
            result[version][ipage]= resultLabel(threshold,diff[version][ipage],pdiff[version][ipage],pixels[ipage]) 

    #save all test files into specified output directory 
    if output_dir:

        #define test sub-directory
        output_subdir= output_dir+'/'+reference.split('.')[0]
        if not os.path.exists(output_subdir): call(['mkdir',output_subdir])
    
        #save test run information into files
        with open(extension(reference,'out'),'w') as f: f.write(stdout)
        with open(extension(reference,'err'),'w') as f: f.write(stderr)
        with open(extension(reference,'usa'),'w') as f: f.write(json.dumps(usage,sort_keys=True,indent=4, separators=(',', ': ')))

        #save parameters as well
        params= {
            'versions':      versions, 
            'interpreter':   interpreter, 
            'executable':    executable, 
            'reference':     reference, 
            'threshold':     threshold, 
            'output_dir':    output_dir,
            'input_dir':     os.path.realpath('.'),
            'branch_name':   branch_name,
            'time':          timestamp,#datetime.now().strftime('%Y%m%d_%H%M%S'),
            'diff':          diff,
            'pdiff':         pdiff,
            'result':        result,
            'pixels':        pixels, 
        }
        with open(extension(reference,'par'),'w') as f: f.write(json.dumps(params,sort_keys=True,indent=4, separators=(',', ': ')))

        #generate an output report
        report= writeHtmlReport(params,usage,stdout,stderr,ref_pages,ref_ver_pages)
        report_filename= extension(reference,'html')        
        with open(report_filename,'w') as f: f.write(report)
        
        #finally, copy the files into output directory
        files_to_copy=  [extension(reference,'html'),
                         extension(reference,'out'),
                         extension(reference,'usa'),
                         extension(reference,'err'),
                         extension(reference,'par')]
        files_to_copy+= ref_pages                                        #output images
        for v in versions:         
            files_to_copy+= ref_ver_pages[v]                             #version reference images
            files_to_copy+= [suffix(e,'_diff') for e in ref_ver_pages[v]]#difference images by ImageMagick
            files_to_copy+= [suffix(e,'_pdif') for e in ref_ver_pages[v]]#difference images by PerceptualDiff
        for filename in files_to_copy:
            target= output_subdir+'/'+filename
            e= call(['scp',filename,target])
            if not e==0:
                sys.stderr.write("ERROR coping the file '%s' into '%s'"%(filename,target))
       

    #compute maximum number of different pixels and percentage...
    max_diff,max_perc = 0,0
    #...for all versions
    for v in diff:
        #...for all pages
        for i in diff[v]:
            d= diff[v][i]
            p= 100.0*d/pixels[i]
            if p>max_perc:
                max_diff= d
                max_perc= p

    #fail if, FOR ANY VERSION OR PAGE, the threshold is passed
    if max_perc>=threshold:
        sys.stderr.write(u"TEST FAILED: Maximum number of different pixels is %d (%.2f%%).\n"%(max_diff,max_perc))
        sys.exit(1)
    else:
        sys.stderr.write(u"TEST OK: Maximum number of different pixels is %d (%.2f%%).\n"%(max_diff,max_perc))
        sys.exit(0)

#####################################################################
#####################################################################
#####################################################################

if __name__ == "__main__":

#####################################################################
#####################################################################
#####################################################################
    
    cmd_parser = OptionParser(usage="usage: %prog <timestamp> <executable> <reference-file>", version='%prog : '+__version__, description = __doc__, prog = 'compare.py')

    print sys.argv#REMOVE??
    
    #flags
    #cmd_parser.add_option("-v", "--verbose", action="store_true", dest="verbose",help="Verbose output while running")
    cmd_parser.add_option("-f", "--force",action="store_true", dest="force",help="Force overwrite of existing output files")
    
    #options
    cmd_parser.add_option("-i", "--interpreter" , default=None,      help="Interpreter command")
    cmd_parser.add_option("-t", "--threshold" ,   default='1.0',     help="Maximum percentage of different pixels")
    cmd_parser.add_option("-n", "--name",         default='NONAME',  help="Testing branch's name")
    cmd_parser.add_option("-v", "--versions",     default=None,      help='List of reference versions, defined as "<ver1> ... <verN>"')
    cmd_parser.add_option("-o", "--output",       default=None,      help="Output report directory (HTML)")

    (optional, positional) = cmd_parser.parse_args()

    #time-stamp
    timestamp= ''
    if positional: timestamp= positional.pop(0)
    try:
        assert(datetime.strptime(timestamp,'%Y%m%d_%H%M%S'))
    except:
        sys.stderr.write(u"Timestamp '%s' is not defined or is not in YYYYMMDD_HHMMSS format.\n"%timestamp)
        sys.exit(1)


    #executable
    executable= ''
    if positional: executable= positional.pop(0)
    try:
        assert(os.path.exists(executable))
    except:
        sys.stderr.write(u"No executable '%s' found.\n"%executable)
        sys.exit(1)

    #reference 
    reference= ''
    if positional:
        reference= positional.pop(0)
    else:
        sys.stderr.write(u"No reference file defined.\n")
        sys.exit(1)
    try:
        assert(optional.force or not os.path.exists(reference))
    except:
        sys.stderr.write(u"Clean file '%s' before running the test. Use -f to force overwrite.\n"%reference)
        sys.exit(1)

    #interpreter
    interpreter= ''
    if optional.interpreter: interpreter= optional.interpreter

    #force
    if optional.force and os.path.exists(reference): call(['rm',reference])

    #threshold    
    threshold= 1.0#default 
    try:
        threshold= float(optional.threshold)
    except:
        sys.stderr.write(u"Invalid threshold '%s': can not convert into float number.\n"%optional.threshold)
        sys.exit(1)

    #name of GIT branch being tested
    branch_name= optional.name 

    #versions
    versions= []
    if optional.versions: versions= optional.versions.strip().split(' ')
    for ver in versions:
        try:
            assert(os.path.exists(ver+'_'+reference))
        except:
            sys.stderr.write(u"No version reference file '%s' found.\n"%(ver+'_'+reference,))
            sys.exit(1)

    #output report directory
    output_dir= optional.output
    if output_dir:
        try:
            assert(os.path.exists(output_dir))
        except:
            sys.stderr.write(u"No output directory '%s' found.\n"%output_dir)
            sys.exit(1)

    compare(timestamp,branch_name,versions,interpreter,executable,reference,threshold,output_dir)
    