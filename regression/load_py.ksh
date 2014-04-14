#!/bin/ksh

versions="current++ new++" 
src=`ls osb_wind.py wind_legend.py`
ext="py"
img_ext="png"
where='wind'
interpretor='python'

dir=`pwd`
name=`basename $dir`


for sf in $src 
do
    s=`basename $sf .$ext`

for v in $versions
do
	echo $v
    if [ $v = "current++" ];
    then
       use magics++ 
	    version=`/usr/local/apps/Magics/$v/bin/magics-config --version`
        ../upload.py $version ./$sf $s.$img_ext magics/reference/$version/$where -i $interpretor
    else
       use newmagics++ 
	   version='2.20.2'
        ../upload.py $version ./$sf $s.$img_ext magics/reference/$version/$where -i $interpretor
	   version='2.22.6'
        ../upload.py $version ./$sf $s.$img_ext magics/reference/$version/$where -i $interpretor
    fi

done
done
