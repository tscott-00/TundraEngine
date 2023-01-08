#pragma once

#include "general_common.h"

#include <string>
#include <unordered_map>
#include <memory>

#include <GL/glew.h>

#include "quad_tree.h"
#include "game_math.h"

// TODO: use macro to do delete copy constructor and assignment operator
// TODO: put more in .cpp and use init lists

class TextureBase {
public:
	static uint64_t activeCirculationCount;
protected:
	// TODO: JUst Box
	struct TextureBox {
		GLuint texture;
		uint64_t circulationID;

		TextureBox(GLuint texture);
		~TextureBox();
	};

	std::shared_ptr<TextureBox> textureBox;

	TextureBase();
	TextureBase(const TextureBase& other);
public:
	TextureBase& operator=(const TextureBase& other);

	// TODO: rtc_GetName
	inline GLuint rtc_GetID() const {
		return textureBox->texture;
	}
};

class Texture2D : public TextureBase {
public:
	struct Descriptor {
		struct Image {
			std::string name;
			int width, height;
			GLint internalFormat;
			GLenum format;
			GLenum type;
			void* data;
			bool dataNeedsToBeLoaded;

			// TODO: If file isn't found, return error tex
			Image(const std::string& fileDirectory, bool srgb = false, bool noAlpha = false);
			Image(int width, int height, GLint internalFormat = GL_RGBA8, GLenum format = GL_RGBA, GLenum type = GL_UNSIGNED_BYTE, void* data = 0);
		private:
			void operator=(const Image& other) = delete;
		} image;
		GLint clamp;
		GLint minFilter, magFilter;
		bool mipmaps;

		Descriptor(const Image& image, bool linear = false, bool mipmaps = false, GLint clamp = GL_CLAMP_TO_EDGE);
	private:
		void operator=(const Descriptor& other) { }
	};

	Texture2D();
	Texture2D(const Descriptor& descriptor);
	Texture2D(const Texture2D& other);

	inline void Bind(unsigned int unit) const {
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, textureBox->texture);
	}

	inline void Bind() const {
		glBindTexture(GL_TEXTURE_2D, textureBox->texture);
	}

	size_t GetTextureBoxPtrInt() const {
		return reinterpret_cast<size_t>(&(*textureBox));
	}
	
	bool operator==(const Texture2D& other) const {
		return textureBox == other.textureBox;
	}
};

namespace std {
	template <>
	struct hash<Texture2D> {
		std::size_t operator()(const Texture2D& k) const {
			return std::hash<size_t>()(k.GetTextureBoxPtrInt());
		}
	};
}

class Texture2DArray : public TextureBase {
public:
	struct Descriptor {
		struct ImageArray {
			GLint internalFormat;
			GLenum format;
			GLenum type;
			bool srgb;

			struct Element {
				// TODO: change to directory, do the same with Texture2D
				std::string name;
				int width, height;
				void* data;
				bool dataNeedsToBeLoaded;

				Element(const std::string& fileDirectory);
			};
			std::list<Element> elements;
			std::string processedDirectory;

			ImageArray(bool srgb);
			ImageArray(std::string processedDirectory, GLint internalFormat, bool srgb);
			ImageArray& AddElement(const Element& element);

			// TODO: If file isn't found, return error tex
		} imageArray;
		GLint clamp;
		GLint minFilter, magFilter;
		bool mipmaps;
		bool anisotropic;

		Descriptor(const ImageArray& imageArray, bool linear = false, bool mipmaps = false, GLint clamp = GL_CLAMP_TO_EDGE);

		Descriptor& WithAnisotropicFiltering();
	private:
		void operator=(const Descriptor& other) = delete;
	};
public:
	Texture2DArray();
	Texture2DArray(const Descriptor& descriptor);
	Texture2DArray(const Texture2DArray& other);

	inline void Bind(unsigned int unit) const {
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D_ARRAY, textureBox->texture);
	}

	inline void Bind() const {
		glBindTexture(GL_TEXTURE_2D_ARRAY, textureBox->texture);
	}
};

class Texture2DQuadAtlas : public TextureBase {
protected:
	struct AtlasData {
		StateQuadTree availabilityTracker;
		std::unordered_map<std::string, FVector4> atlasTexInfoMap;
		unsigned int atlasSize;

		AtlasData(unsigned int initialSize) {
			atlasSize = initialSize;
		}
	};

	std::shared_ptr<AtlasData> atlasData;
public:
	Texture2DQuadAtlas();
	Texture2DQuadAtlas(GLint internalFormat, unsigned int size, bool linear = false);
	Texture2DQuadAtlas(const Texture2DQuadAtlas& other);

	FVector4 GetTexInfo(const std::string& name);

	void SetTexInfoRegion(const std::string& src, const std::string& dest, const FVector4& subregion);

	inline void Bind(unsigned int unit) const {
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_2D, textureBox->texture);
	}

	inline void Bind() const {
		glBindTexture(GL_TEXTURE_2D, textureBox->texture);
	}
protected:
	// TODO: should just take in FVector4 to load tex data in to
	bool AddTexture(const std::string& name, unsigned char* textureData, unsigned int textureSize, unsigned int segmentPosX, unsigned int segmentPosY, unsigned int segmentSize, StateQuadTree* treeNode);
};

class Cubemap : public TextureBase {
public:
	struct Descriptor {
		int width, height;
		GLint internalFormat;
		GLint minFilter, magFilter;
		bool mipmaps;

		Descriptor(int width, int height, GLint internalFormat = GL_RGB8, bool linear = false, bool mipmaps = false);
	private:
		void operator=(const Descriptor& other) = delete;
	};
public:
	Cubemap();
	Cubemap(const Descriptor& descriptor);
	Cubemap(const Cubemap& other);

	inline void Bind(unsigned int unit) const {
		glActiveTexture(GL_TEXTURE0 + unit);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureBox->texture);
	}

	inline void Bind() const {
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureBox->texture);
	}
};
