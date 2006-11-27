The logos have been re-created from the original bitmap based Screenweaver logo. Many thanks to Robert M. Hall for contributing this!

Instructions:

To modify the colors you siply need to change the core asset layer gradient
coors, and then adjust the filter colors on the other layers. For the
outeredge adjust the tint color and value and then you have a brand new
version with new colors.

The steps Robert used to make the icons into the .icns resource file
are as follows:

1) Export from Flash as .pic file with smoothing on (this gets the best
results for me on my mac - for some reason exporting as a 32bit png made the
white panes have rough weird edges. Think this was evident in the .png
preview I sent, but not in the final .icns file)

2) opened in photoshop - removed the surrounding grey by magic wanding the
grey - select inverse, copy, then past into new layer. Remove the old
background layer.

3) Defringed by 1 pixel the new layer to remove bits of white.

4) Exported as a .png with transparency and smoothing on.

5) Dropped the png on a neat mac os x App caled img2icns (
http://www.rknet.it/program/img2icns/)   - I've used iconbuilder photoshop
plugin before - but its overkill most of the time for what I do

6) That's it - it builds the entire icns file for you.