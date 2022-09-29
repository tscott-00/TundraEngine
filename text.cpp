#include "text.h"

#include <fstream>

#include "events_state.h"

namespace
{
	bool StartsWith(const std::string &str, const std::string &smallString)
	{
		if (str.size() < smallString.size())
			return false;

		for (size_t i = 0; i < smallString.size(); ++i)
			if (str[i] != smallString[i])
				return false;

		return true;
	}

	std::string GetAfterChar(const std::string &str, char c)
	{
		for (size_t i = 0; i < str.size(); ++i)
			if (str[i] == c)
				return str.substr(i + 1, str.size());

		return "";
	}

	std::vector<std::string> Split(const std::string &str, char c)
	{
		std::vector<std::string> list;

		std::string build = "";

		for (size_t i = 0; i < str.size(); ++i)
		{
			if (str[i] == c)
			{
				if (build != "")
				{
					list.push_back(build);
					build = "";
				}
			}
			else
				build += str[i];
		}

		if (build != "")
			list.push_back(build);

		return list;
	}

	inline bool is_uppercase(char c)
	{
		return c >= 65 && c <= 90;
	}

	inline char get_lowercase(char c)
	{
		if (is_uppercase(c))
			return c + 32;
		else
			return c;
	}
}

Font::Character::Character() {}

Font::Character::Character(const FVector4 &texCoords, const FVector2 &quadSize, const FVector2 &offsetFromCursor, float cursorXAdvance) : texCoords(texCoords),
																																		  quadSize(quadSize),
																																		  offsetFromCursor(offsetFromCursor),
																																		  cursorXAdvance(cursorXAdvance)
{
}

Font::Font(const std::string &directory) : directory(directory)
{
	for (size_t i = 0; i < (INT8_MAX + 1) * (INT8_MAX + 1); ++i)
		kerningMatrix[i] = 0.0F;

	bool initRecord[INT8_MAX + 1] = {false};

	FVector4 baseTexCoords = Internal::Storage::GameObjects2::texture.GetTexInfo(directory);
	float sizePx = 0.0F;
	float texWidthPx = 0;
	float texHeightPx = 0;

	std::ifstream file;
	std::string fntFileDirectory = "./Resources/" + directory + ".fnt";
	file.open(fntFileDirectory.c_str());

	std::string line;
	if (file.is_open())
	{
		while (file.good())
		{
			getline(file, line);

			if (StartsWith(line, "kerning "))
			{
				std::vector<std::string> vars = Split(line, ' ');
				int firstIndex = std::stoi(GetAfterChar(vars[1], '='));
				int secondIndex = std::stoi(GetAfterChar(vars[2], '='));

				float amountPx = std::stof(GetAfterChar(vars[3], '='));

				kerningMatrix[firstIndex + secondIndex * (INT8_MAX + 1)] = amountPx / sizePx * 2.0F;
			}
			else if (StartsWith(line, "char "))
			{
				std::vector<std::string> vars = Split(line, ' ');
				int index = std::stoi(GetAfterChar(vars[1], '='));
				initRecord[index] = true;

				float xPx = std::stof(GetAfterChar(vars[2], '='));
				float yPx = std::stof(GetAfterChar(vars[3], '='));
				float widthPx = std::stof(GetAfterChar(vars[4], '='));
				float heightPx = std::stof(GetAfterChar(vars[5], '='));
				float offsetFromCursorXPx = std::stof(GetAfterChar(vars[6], '='));
				float offsetFromCursorYPx = std::stof(GetAfterChar(vars[7], '='));
				float cursorXAdvancePx = std::stof(GetAfterChar(vars[8], '='));

				characters[index] = Character(
					FVector4(
						baseTexCoords.x + (xPx / texWidthPx) * baseTexCoords.z,
						baseTexCoords.y + (1.0F - (yPx / texHeightPx) - (heightPx / texHeightPx)) * baseTexCoords.w,
						(widthPx / texWidthPx) * baseTexCoords.z,
						(heightPx / texHeightPx) * baseTexCoords.w),
					FVector2(
						widthPx / sizePx,
						heightPx / sizePx),
					FVector2(
						offsetFromCursorXPx / sizePx * 2.0F,
						-offsetFromCursorYPx / sizePx * 2.0F),
					cursorXAdvancePx / sizePx * 2.0F);
			}
			else if (StartsWith(line, "info"))
			{
				sizePx = std::stof(GetAfterChar(Split(line, ' ')[2], '='));
			}
			else if (StartsWith(line, "common"))
			{
				std::vector<std::string> vars = Split(line, ' ');
				cursorYAdvance = std::stof(GetAfterChar(vars[1], '=')) / sizePx * 2.0F;
				texWidthPx = std::stof(GetAfterChar(vars[3], '='));
				texHeightPx = std::stof(GetAfterChar(vars[4], '='));
			}
		}
	}
	else
	{
		std::cerr << "Unable to load font: " << fntFileDirectory << std::endl;
	}

	for (int i = 0; i <= INT8_MAX; ++i)
		if (initRecord[i] == false)
			characters[i] = characters[127];
}

TextRenderComponent2::MySpecialState::MySpecialState(TextRenderComponent2 *component) : component(component)
{
}

void TextRenderComponent2::MySpecialState::GPUStart()
{
	static Shader shader("Internal/Shaders/text");

	shader.Bind();
	Shader::LoadFloat(0, component->widthDist);
	Shader::LoadFloat(1, component->edgeDist);
	Shader::LoadFloat(2, component->borderWidthDist);
	Shader::LoadFloat(3, component->borderEdgeDist);
	Shader::LoadVector4(4, &component->borderColor.r);
}

void TextRenderComponent2::MySpecialState::GPUStop()
{
	Internal::Storage::GameObjects2::shader.Bind();
}

TextRenderComponent2::TextRenderComponent2(Font *font, int characterSize, const std::string &str) : RenderComponent2(font->directory, FColor::BLACK),
																									font(font),
																									characterSize(characterSize),
																									str(str),
																									widthDist(0.4F),
																									edgeDist(0.4F),
																									borderWidthDist(0.0F),
																									borderEdgeDist(0.0F),
																									borderColor(0.0F, 0.0F, 0.0F, 0.0F),
																									specialState(this),
																									vertices(nullptr),
																									texCoords(nullptr),
																									colors(nullptr),
																									lastBounds(1.0F),
																									cursorPos(-1),
																									lastCursorPos(-1),
																									cursorColor(FColor::WHITE),
																									lastCursorColor(FColor::WHITE)
{
	quadCount = 0;

	charParentTransform = Transform2(FVector2(-1.0F, 1.0F)).WithConstraints(nullptr, nullptr, std::shared_ptr<ScaleConstraint>(new CenterScaleConstraint(static_cast<float>(characterSize))), std::shared_ptr<ScaleConstraint>(new CenterScaleConstraint(static_cast<float>(characterSize))));
}

TextRenderComponent2::TextRenderComponent2(const TextRenderComponent2 &other) : RenderComponent2(other),
																				font(other.font),
																				characterSize(other.characterSize),
																				str(other.str),
																				widthDist(0.4F),
																				edgeDist(0.4F),
																				borderWidthDist(0.0F),
																				borderEdgeDist(0.0F),
																				borderColor(0.0F, 0.0F, 0.0F, 0.0F),
																				specialState(this),
																				vertices(nullptr),
																				texCoords(nullptr),
																				colors(nullptr),
																				lastBounds(1.0F),
																				cursorPos(other.cursorPos),
																				lastCursorPos(other.lastCursorPos),
																				cursorColor(other.cursorColor),
																				lastCursorColor(other.lastCursorColor)
{
	quadCount = 0;

	charParentTransform = Transform2(FVector2(-1.0F, 1.0F)).WithConstraints(nullptr, nullptr, std::shared_ptr<ScaleConstraint>(new CenterScaleConstraint(static_cast<float>(characterSize))), std::shared_ptr<ScaleConstraint>(new CenterScaleConstraint(static_cast<float>(characterSize))));
}

void TextRenderComponent2::OnCreate()
{
	charParentTransform.SetParent(gameObject->transform);
	SetText(str, true);

	WithSpecialState(&specialState);
}

TextRenderComponent2::~TextRenderComponent2()
{
	if (vertices != nullptr)
	{
		delete[] vertices;
		delete[] texCoords;
		delete[] colors;
	}
}

void TextRenderComponent2::SetCharacterSize(int characterSize)
{
	this->characterSize = characterSize;
	charParentTransform = Transform2(FVector2(-1.0F, 1.0F)).WithConstraints(nullptr, nullptr, std::shared_ptr<ScaleConstraint>(new CenterScaleConstraint(static_cast<float>(characterSize))), std::shared_ptr<ScaleConstraint>(new CenterScaleConstraint(static_cast<float>(characterSize))));

	SetText(str, true);
}

void TextRenderComponent2::SetText(const std::string &str, bool force)
{
	if (!force && this->str == str && cursorPos == lastCursorPos && cursorColor == lastCursorColor)
		return;
	lastCursorPos = cursorPos;
	lastCursorColor = cursorColor;

	this->str = str;

	if (vertices != nullptr)
	{
		delete[] vertices;
		delete[] texCoords;
		delete[] colors;
	}

	quadCount = 0;

	struct State
	{
		float fontSize; // Relative to the px size
		float yOffset;
		FColor color;

		State() : fontSize(1.0F),
				  yOffset(0.0F),
				  color(FColor::BLACK)
		{
		}

		bool operator!=(const State &other)
		{
			return fontSize != other.fontSize || yOffset != other.yOffset || color != other.color;
		}
	};

	struct Line
	{
		struct Segment
		{
			struct Image
			{
			public:
				// The char ID coming after this
				int succeedingChar;
				// Horizontal relative size to vertical
				float size;
				FVector4 texCoords;
				FColor color;
				bool drawn;

				Image(int succeedingChar, float size, const FVector4 &texCoords, const FColor &color) : succeedingChar(succeedingChar),
																										size(size),
																										texCoords(texCoords),
																										color(color),
																										drawn(false)
				{
				}
			};

			State state;
			float startCursorPos;
			float localLength;
			std::string value;
			std::vector<Image> images;

			Segment() {}

			Segment(const State &state, float startCursorPos) : state(state),
																startCursorPos(startCursorPos),
																localLength(startCursorPos),
																value("")
			{
			}
		};

		std::vector<Segment> segments;
		float longestLength;
		size_t index;
		float yStart;

		Line() {}

		Line(size_t index, const State &state, float yStart) : longestLength(0.0F),
															   index(index),
															   yStart(yStart)
		{
			segments.push_back(Segment(state, 0.0F));
		}

		Segment &GetSeg()
		{
			return segments[segments.size() - 1];
		}
	};

	struct WordSegment
	{
		State state;
		std::string value;
		float length;
		std::vector<Line::Segment::Image> images;

		WordSegment() {}

		WordSegment(const State &state) : state(state),
										  value(""),
										  length(0.0F)
		{
		}
	};

	struct SavedPos
	{
		float cursorPos;
		size_t line;

		SavedPos() {}

		SavedPos(float cursorPos, size_t line) : cursorPos(cursorPos),
												 line(line)
		{
		}
	};

	std::vector<Line> lines;

	const FMatrix3x3 &transform = gameObject->transform.GetMatrix();

	FVector3 centerPos = transform * FVector3(0.0F, 0.0F, 1.0F);
	FVector3 edgePos = transform * FVector3(1.0F, 1.0F, 1.0F);

	float charRatioX = static_cast<float>(EventsState::windowWidth) / static_cast<float>(characterSize);
	float charRatioY = static_cast<float>(EventsState::windowHeight) / static_cast<float>(characterSize);
	float boundingXVal = (edgePos.x - centerPos.x) * charRatioX * 2.0F + 0.0001F;
	float boundingYVal = -((edgePos.y - centerPos.y) * charRatioY * 2.0F + 0.0001F);

	State currentState;
	std::vector<State> stateStack;
	std::vector<SavedPos> posStack;

	std::vector<WordSegment> currentWord;
	currentWord.push_back(WordSegment(currentState));
	float currentTotalWordLength = 0.0F;
	float currentWordTerminatingSpaceLength = 0.0F;
	float lastWordTerminatingSpaceLength = 0.0F;
	lines.push_back(Line(0, currentState, 0.0F));
	Line *currentLine = &lines[0];

#define add_word_char(c, l)                          \
	currentWord[currentWord.size() - 1].value += c;  \
	currentWord[currentWord.size() - 1].length += l; \
	currentTotalWordLength += l
#define clear_word_value                              \
	currentWord.clear();                              \
	currentWord.push_back(WordSegment(currentState)); \
	currentTotalWordLength = 0.0F;
#define add_current_word_to_current_line                                                                                                                               \
	for (const auto &seg : currentWord)                                                                                                                                \
	{                                                                                                                                                                  \
		if (currentLine->GetSeg().state != seg.state)                                                                                                                  \
			currentLine->segments.push_back(Line::Segment(seg.state, currentLine->GetSeg().localLength));                                                              \
		for (const Line::Segment::Image &img : seg.images)                                                                                                             \
			currentLine->GetSeg().images.push_back(Line::Segment::Image(img.succeedingChar + currentLine->GetSeg().value.size(), img.size, img.texCoords, img.color)); \
		currentLine->GetSeg().value += seg.value;                                                                                                                      \
		currentLine->GetSeg().localLength += seg.length;                                                                                                               \
		if (currentLine->GetSeg().localLength > currentLine->longestLength)                                                                                            \
			currentLine->longestLength = currentLine->GetSeg().localLength;                                                                                            \
	}
#define attempt_to_add_current_line                                                                                      \
	{                                                                                                                    \
		if (currentLine->index == lines.size() - 1)                                                                      \
		{                                                                                                                \
			float upperBounding = -10e10F;                                                                               \
			float lowerBounding = 10e10F;                                                                                \
			for (const auto &seg : currentLine->segments)                                                                \
			{                                                                                                            \
				if (seg.localLength - seg.startCursorPos == 0.0F)                                                        \
					continue;                                                                                            \
				if (-(1.0F - seg.state.fontSize) * 2.0F + seg.state.yOffset > upperBounding)                             \
					upperBounding = -(1.0F - seg.state.fontSize) * 2.0F + seg.state.yOffset;                             \
				if (-(1.0F - seg.state.fontSize) * 2.0F + seg.state.yOffset - seg.state.fontSize * 2.0F < lowerBounding) \
					lowerBounding = -(1.0F - seg.state.fontSize) * 2.0F + seg.state.yOffset - seg.state.fontSize * 2.0F; \
			}                                                                                                            \
			currentLine->yStart -= upperBounding;                                                                        \
			float cursorYAdvance = -font->cursorYAdvance + lowerBounding + 2.0F;                                         \
			if (currentLine->yStart + cursorYAdvance < boundingYVal)                                                     \
			{                                                                                                            \
				lines.pop_back();                                                                                        \
				currentLine = nullptr;                                                                                   \
				goto no_more_lines;                                                                                      \
			}                                                                                                            \
			lines.push_back(Line(lines.size(), currentState, currentLine->yStart + cursorYAdvance));                     \
			currentLine = &lines[lines.size() - 1];                                                                      \
		}                                                                                                                \
		else                                                                                                             \
		{                                                                                                                \
			currentLine = &lines[currentLine->index + 1];                                                                \
		}                                                                                                                \
	}

	bool lastCharWasSpace = false;
	bool canProcessCommands = true;
	bool foundCusor = false;
	if (cursorPos == -1)
		foundCusor = true;
	for (size_t i = 0; i < str.length(); ++i)
	{
		if (canProcessCommands && i < str.length() - 1 && str[i] == '!' && str[i + 1] == '|')
		{
			i += 2;

			std::string commandStr = "";

			while (!(str[i - 1] == '|' && str[i] == '!') && i < str.length())
			{
				if (str[i] != ' ')
					commandStr += get_lowercase(str[i]);
				++i;
			}

			if (!(str[i - 1] == '|' && str[i] == '!'))
				break;

			commandStr = commandStr.substr(0, commandStr.length() - 1);
			std::vector<std::string> commands = Split(commandStr, ';');

			// Invalid commands are ignored
			State newState(currentState);
			for (const std::string &command : commands)
			{
				// Action Commands
				if (command == "quit")
				{
					canProcessCommands = false;

					goto cmd_end;
				}
				else if (command == "pushstate")
					stateStack.push_back(newState);
				else if (command == "popstate")
				{
					if (stateStack.size() > 0)
					{
						newState = stateStack[stateStack.size() - 1];
						stateStack.pop_back();
					}
				}
				else if (command == "pushpos")
					posStack.push_back(SavedPos(currentLine->GetSeg().localLength + lastWordTerminatingSpaceLength + currentTotalWordLength, currentLine->index));
				else if (command == "poppos")
				{
					SavedPos pos = posStack[posStack.size() - 1];
					posStack.pop_back();

					// Pos travel always ends the current word

					if (currentLine->longestLength + lastWordTerminatingSpaceLength + currentTotalWordLength > boundingXVal)
						attempt_to_add_current_line else currentLine->GetSeg().localLength += lastWordTerminatingSpaceLength;

					add_current_word_to_current_line;
					clear_word_value;
					lastWordTerminatingSpaceLength = 0.0F;
					currentWordTerminatingSpaceLength = 0.0F;

					currentLine = &lines[pos.line];
					currentLine->segments.push_back(Line::Segment(currentState, pos.cursorPos));
				}
				else if (command == "slidepos")
				{
					if (currentLine->longestLength + lastWordTerminatingSpaceLength + currentTotalWordLength > boundingXVal)
						attempt_to_add_current_line else currentLine->GetSeg().localLength += lastWordTerminatingSpaceLength;

					add_current_word_to_current_line;
					clear_word_value;
					lastWordTerminatingSpaceLength = 0.0F;
					currentWordTerminatingSpaceLength = 0.0F;

					currentLine->segments.push_back(Line::Segment(currentState, currentLine->longestLength));
				}
				else if (command == "superscript")
					newState.yOffset = newState.yOffset + newState.fontSize * 2.0F;
				else if (command == "subscript")
					newState.yOffset = newState.yOffset - newState.fontSize;
				else if (command == "center")
					newState.yOffset = 1.0F - newState.fontSize;
				// Modify Commands
				else
				{
					std::vector<std::string> commandParts = Split(command, '=');
					if (commandParts.size() != 2)
						continue;

					char opChar = '=';
					if (commandParts[0][commandParts[0].length() - 1] == '*')
					{
						opChar = '*';
						commandParts[0] = commandParts[0].substr(0, commandParts[0].length() - 1);
					}
					else if (commandParts[0][commandParts[0].length() - 1] == '/')
					{
						opChar = '/';
						commandParts[0] = commandParts[0].substr(0, commandParts[0].length() - 1);
					}
					else if (commandParts[0][commandParts[0].length() - 1] == '+')
					{
						opChar = '+';
						commandParts[0] = commandParts[0].substr(0, commandParts[0].length() - 1);
					}
					else if (commandParts[0][commandParts[0].length() - 1] == '-')
					{
						opChar = '-';
						commandParts[0] = commandParts[0].substr(0, commandParts[0].length() - 1);
					}

					float *valuePtr = nullptr;
					if (commandParts[0] == "color")
						valuePtr = &newState.color.r;
					else if (commandParts[0] == "fontsize")
						valuePtr = &newState.fontSize;
					else
						continue;

					// This is an unsafe operation. Modification will not stop if too many values are supplied.
					std::vector<std::string> valueParts = Split(commandParts[1], ',');
					if (opChar == '*')
						for (size_t i = 0; i < valueParts.size(); ++i)
							valuePtr[i] *= stof(valueParts[i]);
					else if (opChar == '/')
						for (size_t i = 0; i < valueParts.size(); ++i)
							valuePtr[i] /= stof(valueParts[i]);
					else if (opChar == '+')
						for (size_t i = 0; i < valueParts.size(); ++i)
							valuePtr[i] += stof(valueParts[i]);
					else if (opChar == '-')
						for (size_t i = 0; i < valueParts.size(); ++i)
							valuePtr[i] -= stof(valueParts[i]);
					// Just a regular set operation
					else
						for (size_t i = 0; i < valueParts.size(); ++i)
							valuePtr[i] = stof(valueParts[i]);
				}
			}
			if (newState != currentState)
			{
				currentState = newState;
				currentWord.push_back(WordSegment(currentState));
			}

		cmd_end:

			lastCharWasSpace = false;

			continue;
		}

		if (str[i] == '\n')
		{
			if (currentLine->longestLength + lastWordTerminatingSpaceLength + currentTotalWordLength > boundingXVal)
				attempt_to_add_current_line else currentLine->GetSeg().localLength += lastWordTerminatingSpaceLength;

			add_current_word_to_current_line;
			clear_word_value;
			lastWordTerminatingSpaceLength = 0.0F;
			currentWordTerminatingSpaceLength = 0.0F;

			attempt_to_add_current_line;

			lastCharWasSpace = false;

			continue;
		}

		if (str[i] == ' ')
		{
			add_word_char(str[i], 0.0F);
			if (!foundCusor && i >= cursorPos)
			{
				currentWord[currentWord.size()-1].images.push_back(Line::Segment::Image(currentWord[currentWord.size()-1].value.length() - 1, 0.15F, Internal::Storage::GameObjects2::texture.GetTexInfo("white"), cursorColor));
				foundCusor = true;
				++quadCount;
			}
			currentWordTerminatingSpaceLength += font->characters[str[i]].cursorXAdvance * currentState.fontSize;

			lastCharWasSpace = true;
		}
		else
		{
			if (lastCharWasSpace)
			{
				// End of word
				if (currentLine->longestLength + lastWordTerminatingSpaceLength + currentTotalWordLength > boundingXVal)
					attempt_to_add_current_line else currentLine->GetSeg().localLength += lastWordTerminatingSpaceLength;

				add_current_word_to_current_line;
				clear_word_value;
				lastWordTerminatingSpaceLength = currentWordTerminatingSpaceLength;
				currentWordTerminatingSpaceLength = 0.0F;
				lastCharWasSpace = false;
			}

			if (currentTotalWordLength + font->characters[str[i]].cursorXAdvance * currentState.fontSize <= boundingXVal)
			{
				add_word_char(str[i], font->characters[str[i]].cursorXAdvance * currentState.fontSize);
				if (!foundCusor && i >= cursorPos)
				{
					currentWord[currentWord.size()-1].images.push_back(Line::Segment::Image(currentWord[currentWord.size()-1].value.length() - 1, 0.15F, Internal::Storage::GameObjects2::texture.GetTexInfo("white"), cursorColor));
					foundCusor = true;
					++quadCount;
				}
				++quadCount;
			}
			else
			{
				// Jump to the end of the word, skipping everything that can't fit on an independent line
				while (str[i] != ' ' && i < str.length())
					++i;
				--i;

				continue;
			}
		}
	}

	if (!foundCusor)
	{
		currentWord[currentWord.size()-1].images.push_back(Line::Segment::Image(currentWord[currentWord.size()-1].value.length(), 0.15F, Internal::Storage::GameObjects2::texture.GetTexInfo("white"), cursorColor));
		foundCusor = true;
		++quadCount;
	}

	if (currentLine->longestLength + lastWordTerminatingSpaceLength + currentTotalWordLength > boundingXVal)
	{
		// New line
		attempt_to_add_current_line;
		add_current_word_to_current_line;
		attempt_to_add_current_line;
	}
	else
	{
		currentLine->GetSeg().localLength += lastWordTerminatingSpaceLength;

		add_current_word_to_current_line;
		attempt_to_add_current_line;
	}

no_more_lines:

	FVector2 cursorPos(0.0F, 0.0F);
	vertices = new FVector2[quadCount * 4];
	texCoords = new FVector2[quadCount * 4];
	colors = new FColor[quadCount * 4];
	size_t vertexPtr = 0;

	for (Line &line : lines)
	{
		cursorPos.y = line.yStart;

		// Cancel out first kerning [?]
		float lineXStart = -10e10F;
		for (const Line::Segment &segment : line.segments)
			if (segment.startCursorPos == 0.0F && -font->characters[segment.value[0]].offsetFromCursor.x > lineXStart)
				lineXStart = -font->characters[segment.value[0]].offsetFromCursor.x;

		for (Line::Segment &segment : line.segments)
		{
			cursorPos.x = segment.startCursorPos + lineXStart;
			float segmentYOffset = -(1.0F - segment.state.fontSize) * 2.0F + segment.state.yOffset;

			char lastChar = ' ';
			for (size_t i = 0; i < segment.value.length(); ++i)
			{
				Font::Character &c = font->characters[segment.value[i]];

				// TODO: Also do after loop for ones the size of the list
				for (Line::Segment::Image &img : segment.images)
					if (img.succeedingChar == i)
					{
						img.drawn = true;
						//std::cerr << "cposdraw " << i << " " << segment.value[i] << std::endl;
						FVector2 quadSize(img.size, 1.0F);
						FVector2 pos = cursorPos + FVector2(0.0F, -1.0F * segment.state.fontSize);
						vertices[vertexPtr] = pos + UNIT_UPPER_RIGHT * quadSize * segment.state.fontSize;
						vertices[vertexPtr + 1] = pos + UNIT_UPPER_LEFT * quadSize * segment.state.fontSize;
						vertices[vertexPtr + 2] = pos + UNIT_LOWER_LEFT * quadSize * segment.state.fontSize;
						vertices[vertexPtr + 3] = pos + UNIT_LOWER_RIGHT * quadSize * segment.state.fontSize;

						texCoords[vertexPtr].x = img.texCoords.x + img.texCoords.z;
						texCoords[vertexPtr].y = img.texCoords.y + img.texCoords.w;
						texCoords[vertexPtr + 1].x = img.texCoords.x;
						texCoords[vertexPtr + 1].y = img.texCoords.y + img.texCoords.w;
						texCoords[vertexPtr + 2].x = img.texCoords.x;
						texCoords[vertexPtr + 2].y = img.texCoords.y;
						texCoords[vertexPtr + 3].x = img.texCoords.x + img.texCoords.z;
						texCoords[vertexPtr + 3].y = img.texCoords.y;

						colors[vertexPtr] = img.color;
						colors[vertexPtr + 1] = img.color;
						colors[vertexPtr + 2] = img.color;
						colors[vertexPtr + 3] = img.color;

						vertexPtr += 4;

						cursorPos.x += img.size * segment.state.fontSize;
					}

				if (segment.value[i] != ' ')
				{
					// Font considers the cursor to be at the top of the letter space
					cursorPos.x += font->kerningMatrix[lastChar + segment.value[i] * (INT8_MAX + 1)] * segment.state.fontSize;
					FVector2 pos = cursorPos;
					pos.x += c.quadSize.x * segment.state.fontSize + c.offsetFromCursor.x * segment.state.fontSize;
					pos.y += -c.quadSize.y * segment.state.fontSize + c.offsetFromCursor.y * segment.state.fontSize + segmentYOffset;
					vertices[vertexPtr] = pos + UNIT_UPPER_RIGHT * c.quadSize * segment.state.fontSize;
					vertices[vertexPtr + 1] = pos + UNIT_UPPER_LEFT * c.quadSize * segment.state.fontSize;
					vertices[vertexPtr + 2] = pos + UNIT_LOWER_LEFT * c.quadSize * segment.state.fontSize;
					vertices[vertexPtr + 3] = pos + UNIT_LOWER_RIGHT * c.quadSize * segment.state.fontSize;

					texCoords[vertexPtr].x = c.texCoords.x + c.texCoords.z;
					texCoords[vertexPtr].y = c.texCoords.y + c.texCoords.w;
					texCoords[vertexPtr + 1].x = c.texCoords.x;
					texCoords[vertexPtr + 1].y = c.texCoords.y + c.texCoords.w;
					texCoords[vertexPtr + 2].x = c.texCoords.x;
					texCoords[vertexPtr + 2].y = c.texCoords.y;
					texCoords[vertexPtr + 3].x = c.texCoords.x + c.texCoords.z;
					texCoords[vertexPtr + 3].y = c.texCoords.y;

					colors[vertexPtr] = segment.state.color;
					colors[vertexPtr + 1] = segment.state.color;
					colors[vertexPtr + 2] = segment.state.color;
					colors[vertexPtr + 3] = segment.state.color;

					vertexPtr += 4;
				}
				lastChar = segment.value[i];

				cursorPos.x += c.cursorXAdvance * segment.state.fontSize;
			}

			for (const Line::Segment::Image &img : segment.images)
				if (!img.drawn)
				{
					FVector2 quadSize(img.size, 1.0F);
					FVector2 pos = cursorPos + FVector2(0.0F, -1.0F * segment.state.fontSize);
					vertices[vertexPtr] = pos + UNIT_UPPER_RIGHT * quadSize * segment.state.fontSize;
					vertices[vertexPtr + 1] = pos + UNIT_UPPER_LEFT * quadSize * segment.state.fontSize;
					vertices[vertexPtr + 2] = pos + UNIT_LOWER_LEFT * quadSize * segment.state.fontSize;
					vertices[vertexPtr + 3] = pos + UNIT_LOWER_RIGHT * quadSize * segment.state.fontSize;

					texCoords[vertexPtr].x = img.texCoords.x + img.texCoords.z;
					texCoords[vertexPtr].y = img.texCoords.y + img.texCoords.w;
					texCoords[vertexPtr + 1].x = img.texCoords.x;
					texCoords[vertexPtr + 1].y = img.texCoords.y + img.texCoords.w;
					texCoords[vertexPtr + 2].x = img.texCoords.x;
					texCoords[vertexPtr + 2].y = img.texCoords.y;
					texCoords[vertexPtr + 3].x = img.texCoords.x + img.texCoords.z;
					texCoords[vertexPtr + 3].y = img.texCoords.y;

					colors[vertexPtr] = img.color;
					colors[vertexPtr + 1] = img.color;
					colors[vertexPtr + 2] = img.color;
					colors[vertexPtr + 3] = img.color;

					vertexPtr += 4;

					cursorPos.x += img.size * segment.state.fontSize;
				}
		}
	}
}

TextRenderComponent2 *TextRenderComponent2::WithBorder(const FColor &color)
{
	EnableBorder(color);

	return this;
}

void TextRenderComponent2::SetBorder(float widthDist, float edgeDist, float borderWidthDist, float borderEdgeDist, const FColor &borderColor)
{
	this->widthDist = widthDist;
	this->edgeDist = edgeDist;
	this->borderWidthDist = borderWidthDist;
	this->borderEdgeDist = borderEdgeDist;
	this->borderColor = borderColor;
}

void TextRenderComponent2::EnableBorder(const FColor &color)
{
	widthDist = 0.45F;
	edgeDist = 0.35F;
	borderWidthDist = 0.7F;
	borderEdgeDist = 0.3F;
	borderColor = color;
}

void TextRenderComponent2::DisableBorder()
{
	widthDist = 0.4F;
	edgeDist = 0.4F;
	borderWidthDist = 0.0F;
	borderEdgeDist = 0.0F;
	borderColor = FColor(0.0F, 0.0F, 0.0F, 0.0F);
}

void TextRenderComponent2::LoadData(FVector2 *vertices, FVector2 *texCoords, FColor *colors)
{
	const FMatrix3x3 &matrix = charParentTransform.GetMatrix();
	for (unsigned short i = 0; i < quadCount * 4; ++i)
	{
		vertices[i] = Transform(this->vertices[i], matrix);
		texCoords[i] = this->texCoords[i];
		colors[i] = this->colors[i];
	}
}