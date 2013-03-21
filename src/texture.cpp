#include "texture.h"
#include "lodepng.h"


Texture::Texture(const std::string &filename, const GLint filter_param)
{

	//haidi haida. oikea kurahaara ja jakorasia.

	// LODEPNG STUFF!

	std::vector<unsigned char> raw_pixels;
	unsigned width, height;
	unsigned error = lodepng::decode(raw_pixels, width, height, filename);
	
	if(error) { logWindowOutput( "lodepng decoder error: %s\n", lodepng_error_text(error)); _otherbad = true; return; }
	
	name = filename;

	/*std::ifstream infile(filename, std::ios::in | std::ios::binary);

	if (!infile.is_open()) {
		_nosuch = true;
		return;
	}

	char *buffer = NULL;

	decompress_qlz(infile, &buffer);
	infile.close();
	
	_badheader = _nosuch = _otherbad = false;
	
	if (!((*buffer) == 'B' && (*(buffer+1) == 'M')))
	{
		delete [] buffer;
		_badheader = true;
		return;
	}
	*/
	
	_badheader = _nosuch = _otherbad = false;
	// start reading header file.

	// the windows BMP file has a 54-byte header

	/*char* iter = buffer + 2;	// the first two bytes are 'B' and 'M', and this we already have checked.

	BMPHEADER header;

	memcpy(&header, iter, sizeof(header));
	
	// validate image OpenGL-wise

	if (header.width == header.height) 
	{
		if ((header.width & (header.width - 1)) == 0)	// if power of two
		{ */
	if ((width & (width - 1)) == 0 && width == height) {
			// image is valid, carry on
			hasAlpha = true;	// lodepng loads as RGBARGBA...

			// read actual image data to buffer.
		//	GLbyte *const imagedata = (GLbyte*)(buffer + 54);
			glEnable(GL_TEXTURE_2D);
			glGenTextures(1, &textureId);
			glBindTexture( GL_TEXTURE_2D, textureId);
			glTexStorage2D(GL_TEXTURE_2D, 4, GL_RGBA8, width, height);
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &raw_pixels[0]);
			glGenerateMipmap(GL_TEXTURE_2D);	// requires glew
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter_param);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

			//glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, imagedata);
			//glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
			
			//glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR );


			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		}
	
		else {	// if not power of two
			_otherbad = true;
		}
}

	/*****
	* REMEMBER: THE OPENGL CONTEXT MUST BE ALREADY CREATED BEFORE ANY OF THE gl<WHATEVER> class take place
	*/

std::vector<Texture> TextureBank::textures;

GLint TextureBank::get_id_by_name(const std::string &name) {
	std::vector<Texture>::const_iterator iter = textures.begin();
	while (iter != textures.end()) {
		const Texture &t = (*iter);
		if (t.getName() == name) {
			return t.getId();
		}
		++iter;
	}
	return -1;
}

void TextureBank::add(const Texture &t) {
	textures.push_back(t);
}

bool TextureBank::validate() {
			
			std::vector<Texture>::const_iterator iter;
			bool badvalues = false;
			for(iter = textures.begin(); iter != textures.end(); ++iter)
			{	
				const Texture &t = (*iter);
				if (t.bad()) {
					badvalues = true;
					logWindowOutput( "[Textures] invalid textures detected:\n");
					
					if (t.badheader()) {
						logWindowOutput( "%s: bad file header.\n", t.getName().c_str()); }
					
					else if (t.nosuch())
						logWindowOutput( "%s: no such file or directory.\n", t.getName().c_str());

					else if (t.otherbad())
						logWindowOutput( "%s: file either is not square (n-by-n), or not power of two (128x128, 256x256 etc.)\n\n", t.getName().c_str());
					}
					
			}

			return !badvalues;
}




