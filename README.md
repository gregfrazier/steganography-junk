# steganography-junk
Messing around with steganography by embedding files into images.

Can you spot the difference?
![alt tag](https://raw.githubusercontent.com/gregfrazier/steganography-junk/master/example-stegano.png)

The image on the left is just a picture of some trees.

The image on the right is also just a picture of some trees.

Yet one of them is hiding a secret. Trapped in the image is a tar.gz file of the source code to this repository.

How it works
----
It's really simple, basically you find an image and convert it to 24bpp BMP file with no extra header info (tell your editor not to save extra stuff) and then insert another file into it by dispersing the file's bit evenly throughout the image so that it does not drastically modify the content at a perceivable level. This is implemented in 'junk.cpp'

This doesn't work with all images, black and white images tend to get a little muddy and well as images consisting of the same color. The best ones to use are photographs, with varying levels of color and shading.

Then you extract the file by using the original image as a key. This is implemented in 'unjunk.cpp'

File Formats
----
This application uses 24bpp BMP files because they are a lossless format that's easy to read. It can be implemented with any lossless format, such as PNG, TGA, etc. Lossy formats, like JPEG and GIF can't be used because of the implementation style. 

Without modification, this program can embed into a BMP, then compressed with a lossless algorithm without loss of data. Convert back to BMP before unjunkifying it.
