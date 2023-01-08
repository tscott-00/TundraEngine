#include "texture.h"

#include "debug.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>

uint64_t TextureBase::activeCirculationCount = UINT64_C(0);

namespace {
	float SGLToLinear(float s) {
		if (s <= 0.04045F)
			return s / 12.92F;
		else
			return powf((s + 0.055F) / 1.055F, 2.4F);
	}
}

TextureBase::TextureBox::TextureBox(GLuint texture) :
	texture(texture) {
	static uint64_t totalCirculationIDCount = UINT64_C(0);
	circulationID = totalCirculationIDCount++;
	++activeCirculationCount;
}

TextureBase::TextureBox::~TextureBox() {
	--activeCirculationCount;
	glDeleteTextures(1, &texture);
}

TextureBase::TextureBase() :
	textureBox(nullptr) { }

TextureBase::TextureBase(const TextureBase& other) :
	textureBox(other.textureBox) { }

TextureBase& TextureBase::operator=(const TextureBase& other) {
	textureBox = other.textureBox;

	return *this;
}

Texture2D::Descriptor::Image::Image(const std::string& fileDirectory, bool srgb, bool noAlpha) {
	this->name = fileDirectory;
	dataNeedsToBeLoaded = true;

	width = 0;
	height = 0;
	internalFormat = srgb ? (noAlpha ? GL_SRGB8 : GL_SRGB8_ALPHA8) : (noAlpha ? GL_RGB8 : GL_RGBA8);
	format = GL_RGBA;
	type = GL_UNSIGNED_BYTE;
	data = 0;
}

Texture2D::Descriptor::Image::Image(int width, int height, GLint internalFormat, GLenum format, GLenum type, void* data) :
	width(width),
	height(height),
	internalFormat(internalFormat),
	format(format),
	type(type),
	data(data),
	dataNeedsToBeLoaded(false) { }

Texture2D::Descriptor::Descriptor(const Image& image, bool linear, bool mipmaps, GLint clamp) :
	image(image),
	clamp(clamp),
	mipmaps(mipmaps),
	minFilter(mipmaps ? GL_LINEAR_MIPMAP_LINEAR : (linear ? GL_LINEAR : GL_NEAREST)),
	magFilter(linear ? GL_LINEAR : GL_NEAREST) { }

Texture2D::Texture2D() { }

Texture2D::Texture2D(const Descriptor& descriptor) {
	if ((descriptor.image.width == 0 || descriptor.image.height == 0) && !descriptor.image.dataNeedsToBeLoaded) {
		Console::Error("Texture2D " + descriptor.image.name + " has size of 0,0");
		return;
	}

	void* data = descriptor.image.data;
	int width = descriptor.image.width;
	int height = descriptor.image.height;
	if (descriptor.image.dataNeedsToBeLoaded) {
		int numComponents;

		stbi_set_flip_vertically_on_load(true);
		data = stbi_load(("./Resources/" + descriptor.image.name + ".png").c_str(), &width, &height, &numComponents, 4);
		//std::cerr << "loaded " << fileDirectory << " " << std::to_string(data) << std::endl;
		// WARNING: does numComponents need to be checked?

		if (data == nullptr)
			Console::Error("Image loading failed for: " + descriptor.image.name + ".png");
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, descriptor.clamp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, descriptor.clamp);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, descriptor.minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, descriptor.magFilter);
	glTexImage2D(GL_TEXTURE_2D, 0, descriptor.image.internalFormat, width, height, 0, descriptor.image.format, descriptor.image.type, data);
	if (descriptor.mipmaps)
		glGenerateMipmap(GL_TEXTURE_2D);

	if (descriptor.image.dataNeedsToBeLoaded)
		stbi_image_free(data);

	textureBox = std::shared_ptr<TextureBox>(new TextureBox(texture));
}

Texture2D::Texture2D(const Texture2D& other) : TextureBase(other) { }

Texture2DArray::Descriptor::ImageArray::Element::Element(const std::string& fileDirectory) :
	name(fileDirectory),
	dataNeedsToBeLoaded(true),
	width(0),
	height(0),
	data(0) { }

Texture2DArray::Descriptor::ImageArray::ImageArray(bool srgb) :
	processedDirectory(""),
	srgb(srgb),
	internalFormat(GL_RGBA8),
	format(GL_RGBA),
	type(GL_UNSIGNED_BYTE) { }

Texture2DArray::Descriptor::ImageArray::ImageArray(std::string processedDirectory, GLint internalFormat, bool srgb) :
	processedDirectory(processedDirectory),
	internalFormat(internalFormat),
	srgb(srgb),
	format(GL_RGBA),
	type(GL_UNSIGNED_BYTE) { }

Texture2DArray::Descriptor::ImageArray& Texture2DArray::Descriptor::ImageArray::AddElement(const Element& element) {
	if ((element.width == 0 || element.height == 0) && !element.dataNeedsToBeLoaded) {
		Console::Error("Texture Array element needs to have a size larger than 0,0");
		return *this;
	}
	elements.push_back(element);
	return *this;
}

Texture2DArray::Descriptor::Descriptor(const ImageArray& imageArray, bool linear, bool mipmaps, GLint clamp) :
	imageArray(imageArray),
	anisotropic(false),
	clamp(clamp),
	mipmaps(mipmaps),
	minFilter(mipmaps ? GL_LINEAR_MIPMAP_LINEAR : (linear ? GL_LINEAR : GL_NEAREST)),
	magFilter(linear ? GL_LINEAR : GL_NEAREST) { }

Texture2DArray::Descriptor& Texture2DArray::Descriptor::WithAnisotropicFiltering() {
	anisotropic = true;
	return *this;
}

Texture2DArray::Texture2DArray() { }

Texture2DArray::Texture2DArray(const Texture2DArray& other) : TextureBase(other) { }

Texture2DArray::Texture2DArray(const Descriptor& descriptor) {
	if (descriptor.imageArray.elements.size() == 0 && descriptor.imageArray.processedDirectory == "") {
		Console::Error("Texture2D Array has 0 layers");
		return;
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D_ARRAY, texture);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, descriptor.clamp);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, descriptor.clamp);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, descriptor.minFilter);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, descriptor.magFilter);
	if (descriptor.anisotropic) {
		GLfloat largest;
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &largest);
		glTexParameterf(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_ANISOTROPY, largest);
	}

	if (descriptor.imageArray.processedDirectory == "") {
		int lastWidth = -1;
		int lastHeight = -1;
		uint8_t* all = nullptr;
		int maxIndex = 0;
		int j = 0;
		for (const Descriptor::ImageArray::Element& element : descriptor.imageArray.elements) {
			// TODO: fix that images not from disk can't be loaded (element.dataNeedsToBeLoaded = false will break everything)
			uint8_t* data = nullptr;
			int width = element.width;
			int height = element.height;
			if (element.dataNeedsToBeLoaded) {
				int numComponents;

				stbi_set_flip_vertically_on_load(true);
				data = stbi_load(("./Resources/" + element.name + ".png").c_str(), &width, &height, &numComponents, 4);
				//std::cerr << "loaded " << fileDirectory << " " << std::to_string(data) << std::endl;
				// WARNING: does numComponents need to be checked?

				if (data == nullptr)
					Console::Error("Image loading failed for: " + element.name + ".png");
			}
			if (lastWidth == -1) {
				lastWidth = width;
				lastHeight = height;
				maxIndex = width * height * 4;
				all = new uint8_t[maxIndex * descriptor.imageArray.elements.size()];
				if (width == 0 || height == 0)
					Console::Error("Size of first layer has size of zero");
			} else if (lastWidth != width || lastHeight != height) {
				Console::Error("Size of this layer (" + std::to_string(width) + "," + std::to_string(height) + ") does not match size of last layer (" + std::to_string(width) + "," + std::to_string(height) + ")");
			}

			//memcpy(all, data, sizeof(uint8_t) * 4 * maxIndex);
			if (descriptor.imageArray.srgb) {
				for (int i = 0; i < maxIndex; ++i)
					if (i % 4 != 3)
						all[i + j * maxIndex] = static_cast<uint8_t>(floorf(SGLToLinear(data[i] / 255.0F) * 255.0F));
					else
						all[i + j * maxIndex] = data[i];
			} else {
				for (int i = 0; i < maxIndex; ++i)
					all[i + j * maxIndex] = data[i];
			}
			if (element.dataNeedsToBeLoaded)
				stbi_image_free(data);
			++j;
		}

		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, descriptor.imageArray.internalFormat, lastWidth, lastHeight, static_cast<GLsizei>(descriptor.imageArray.elements.size()), 0, descriptor.imageArray.format, descriptor.imageArray.type, all);
		delete[] all;
	} else {
		uint8_t* data;
		int width;
		int height;
		int numComponents;

		stbi_set_flip_vertically_on_load(true);
		data = stbi_load(("./Resources/" + descriptor.imageArray.processedDirectory + ".png").c_str(), &width, &height, &numComponents, 4);

		if (data == nullptr)
			Console::Error("Image array loading failed for: " + descriptor.imageArray.processedDirectory + ".png");

		if (descriptor.imageArray.srgb) {
			for (int i = 0; i < width * height * 4; ++i)
				if (i % 4 != 3)
					data[i] = static_cast<uint8_t>(floorf(SGLToLinear(data[i] / 255.0F) * 255.0F));
				else
					data[i] = data[i];
		}

		glTexImage3D(GL_TEXTURE_2D_ARRAY, 0, descriptor.imageArray.internalFormat, width, width, height / width, 0, descriptor.imageArray.format, descriptor.imageArray.type, data);

		stbi_image_free(data);
	}

	if (descriptor.mipmaps)
		glGenerateMipmap(GL_TEXTURE_2D_ARRAY);

	textureBox = std::shared_ptr<TextureBox>(new TextureBox(texture));
}

Texture2DQuadAtlas::Texture2DQuadAtlas() { }

Texture2DQuadAtlas::Texture2DQuadAtlas(GLint internalFormat, unsigned int initialSize, bool linear) {
	if (initialSize == 0) {
		Console::Error("Texture2DQuadAtlas has initial size of 0");
		return;
	}

	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, initialSize, initialSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	textureBox = std::shared_ptr<TextureBox>(new TextureBox(texture));
	atlasData = std::shared_ptr<AtlasData>(new AtlasData(initialSize));
}

Texture2DQuadAtlas::Texture2DQuadAtlas(const Texture2DQuadAtlas& other) :
	TextureBase(other),
	atlasData(other.atlasData) { }

bool Texture2DQuadAtlas::AddTexture(const std::string& name, unsigned char* textureData, unsigned int textureSize, unsigned int segmentPosX, unsigned int segmentPosY, unsigned int segmentSize, StateQuadTree* treeNode) {
	if (treeNode->GetState() == false || textureSize > segmentSize)
		return false;

	if (textureSize == segmentSize && (*treeNode)[0] == nullptr && (*treeNode)[1] == nullptr && (*treeNode)[2] == nullptr && (*treeNode)[3] == nullptr) {
		Bind();
		glTexSubImage2D(GL_TEXTURE_2D, 0, segmentPosX, segmentPosY, textureSize, textureSize, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
		float totalSize = static_cast<float>(atlasData->atlasSize);
		atlasData->atlasTexInfoMap[name] = FVector4(segmentPosX / totalSize, segmentPosY / totalSize, textureSize / totalSize, textureSize / totalSize);
		treeNode->FalsifyState();
		return true;
	} else if (segmentSize > 1) {
		unsigned int newSize = segmentSize / 2;
		if ((*treeNode)[0] == nullptr)
			treeNode->Subdivide();
		for (unsigned int i = 0; i < StateQuadTree::CHILDREN_COUNT; ++i)
			if (AddTexture(name, textureData, textureSize, segmentPosX + (i % 2) * newSize, segmentPosY + (i / 2) * newSize, newSize, (*treeNode)[i]))
				return true;
	}

	return false;
}

FVector4 Texture2DQuadAtlas::GetTexInfo(const std::string& name) {
	auto info = atlasData->atlasTexInfoMap.find(name);
	if (info == atlasData->atlasTexInfoMap.end()) {
		int width;
		int height;

		int numComponents;

		stbi_set_flip_vertically_on_load(true);
		unsigned char* data = stbi_load(("./Resources/" + name + ".png").c_str(), &width, &height, &numComponents, 4);

		if (width != height) {
			Console::Error("Unable to add texture '" + name + "' to atlas as it does not have square dimensions");
			return FVector4(0.0F, 0.0F, 1.0F, 1.0F);
		}

		if (!AddTexture(name, data, width, 0, 0, atlasData->atlasSize, &atlasData->availabilityTracker))
			Console::Error("Unable to find position to add texture '" + name + "' to atlas");

		stbi_image_free(data);

		info = atlasData->atlasTexInfoMap.find(name);
		if (info == atlasData->atlasTexInfoMap.end()) {
			// Only if texture addition failed
			return FVector4(0.0F, 0.0F, 1.0F, 1.0F);
		}
	}

	return info->second;
}

void Texture2DQuadAtlas::SetTexInfoRegion(const std::string& src, const std::string& dest, const FVector4& subregion) {
	FVector4 base = GetTexInfo(src);
	atlasData->atlasTexInfoMap[dest] = FVector4(base.x + subregion.x * base.z, base.y + subregion.y * base.w, subregion.z * base.z, subregion.w * base.w);
}

Cubemap::Descriptor::Descriptor(int width, int height, GLint internalFormat, bool linear, bool mipmaps) :
	width(width),
	height(height),
	internalFormat(internalFormat),
	mipmaps(mipmaps),
	minFilter(mipmaps ? GL_LINEAR_MIPMAP_LINEAR : (linear ? GL_LINEAR : GL_NEAREST)),
	magFilter(linear ? GL_LINEAR : GL_NEAREST) { }

Cubemap::Cubemap() { }

Cubemap::Cubemap(const Descriptor& descriptor) {
	GLuint texture;
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, texture);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, descriptor.minFilter);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, descriptor.magFilter);

	for (GLuint i = 0; i < 6; ++i)
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, descriptor.internalFormat, descriptor.width, descriptor.height, 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);

	if (descriptor.mipmaps)
		glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	textureBox = std::shared_ptr<TextureBox>(new TextureBox(texture));
}

Cubemap::Cubemap(const Cubemap& other) : TextureBase(other) { }
