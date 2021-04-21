#!/usr/bin/env bash

##let CHARS_PER_FONT=95
let CHARS_PER_FONT=224
## Padding:: for i in $(seq 095 0222); do NUM=$(printf "%03d" $i); cp part-000.xbm part-$NUM.xbm; done

###
## Check if we have convert installed
##
###
function SanityCheck() {
    tools=("convert")

    for t in ${tools[@]}; do
	if [ ! -x "$(command -v "$t")" ]; then
	    echo "You need to have ImageMagick installed since we're using 'convert' tool from it."
	    exit 1
	fi
    done
}

### 
##  Split font file to characters into font file subdir
##
##  $1 is XBM font file
###
function SplitFont() {
    fontFile="$1"

    if [ ! -f $fontFile ]; then
	echo "Font file '$fontFile' doesn't exist."
	return;
    fi

    fontDir=$(basename $fontFile ".xbm")
    echo "Font Directory: $fontDir"
    
    let width=$(grep _width $fontFile | awk 'BEGIN { FS=" " }  { print $3; }')
    let height=$(grep _height $fontFile | awk 'BEGIN { FS=" " }  { print $3; }')
    ##let dataLines=$(grep "^[[:blank:]]*0x" $fontFile | wc -l)

    let charWidth=$((height / CHARS_PER_FONT))
    charSize="${charWidth}x${width}"

    echo "Character size: $charSize"

    rm -rf "$fontDir"
    mkdir $fontDir

    ##cp $fontFile $fontDir
    ##convert "${fontFile}[32-$((CHARS_PER_FONT + 32 - 1))]" -scene $((256 - CHARS_PER_FONT)) -rotate 270 -crop $charSize $fontDir/part-%03d.xbm
    convert "${fontFile}" -scene $((256 - CHARS_PER_FONT)) -rotate 270 -crop $charSize $fontDir/part-%03d.xbm
    let ret=$?
    
    if [ $ret -eq 0 ]; then
	echo "Font file successfuly splitted into $fontDir."
    else
	echo "Something went wrong while splitting font file."
    fi
    echo ""
}

###
##
##
##  Merge font characters from font dir to a single XBM file
##
##  $1 is source XMB directory
###
function MergeFont() {
    fontDir="$1"

    if [ ! -d $fontDir ]; then
	echo "Font directory '$fontDir' doesn't exist."
	return;
    fi

    fontFile="${fontDir}-to_import.xbm"
    
    let maxChar=$((32 + CHARS_PER_FONT - 1))
    convert $(eval echo $fontDir/part-{032..$maxChar}.xbm) +append -rotate 90 $fontFile
    let ret=$?
    
    if [ $ret -eq 0 ]; then
	echo "Font file $fontFile successfuly created."
    else
	echo "Something went wrong while buidling font file."
    fi
    echo ""
}

###
##
##
###
function DisplayHelp() {
    progName=$(basename $0)
    echo "$progName v0.0.1"
    echo ""
    echo "Usage:"
    echo "     -h, --help                    : display this help message."
    echo "     -s, --split <font_file.xbm>   : extract all characters image from specified file into <font_file>/ subdirectory."
    echo "     -m, --merge <font_dir>        : build font file from characters files located into <font_dir>."
    echo ""
}

###
##
## Main
###

SanityCheck

if [ $# -eq 0 ]; then
    DisplayHelp
    exit 1
fi

while [[ $# -gt 0 ]]; do
    key="$1"
    
    case "$key" in
	"--split"|"-s")
	    if [ "$#" -lt 2 -o -z "$(( $# % 2 ))" ]; then
		echo "One argument is missing...."
		DisplayHelp
	    fi
	    shift
	    SplitFont "$1"
	    shift
	    ;;
	
	"--merge"|"-m")
	    if [ "$#" -lt 2 -o -z "$(( $# % 2 ))" ]; then
		echo "One argument is missing...."
		DisplayHelp
	    fi
	    shift
	    MergeFont "$1"
	    shift
	    ;;
	
	"--help"|"-h")
	    DisplayHelp
	    exit 0
	    ;;
	
	*)
	    echo "Missing or unknown argument $key."
	    DisplayHelp
	    exit 1
	    ;;
    esac
done

exit 0
