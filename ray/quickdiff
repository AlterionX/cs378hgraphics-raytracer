#!/bin/bash

#Make sure correct file hierarchies exist
if [ ! -d raycheck.out/image ]; then
    echo "Please make sure raycheck.out exists!"
    exit -1
fi

# Copy directory tree structure to new folders
rsync -a -f"+ */" -f"- *" raycheck.out/image/ raycheck.out/diffs
rsync -a -f"+ */" -f"- *" raycheck.out/image/ raycheck.out/montage

# Process
for file in $(find raycheck.out/image/ -type f); do
    FULL_FNAME="$(realpath --relative-to="raycheck.out/image/" $file)"
    FNAME=${FULL_FNAME%.*}

    echo "Processing $FNAME"

    IMGNAME=raycheck.out/image/"$FNAME".png
    REFNAME=raycheck.out/refcache/"$FNAME".std.png
    OUTNAME=raycheck.out/diffs/"$FNAME".diff.png
    DIFNAME=raycheck.out/montage/"$FNAME".mont.png

    compare "$IMGNAME" "$REFNAME" "$OUTNAME"
    montage "$IMGNAME" "$REFNAME" "$OUTNAME" -tile 3x1 -geometry +0+0 "$DIFNAME"
done

trap "echo Quitting...; exit -2" SIGINT SIGTERM
