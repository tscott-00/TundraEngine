#include "ui.h"

#include <GL/glew.h>

#include "rendering.h"
#include "events_state.h"
#include "input.h"

const FVector2 RenderComponent2::UNIT_UPPER_RIGHT = FVector2(1.0F, 1.0F);
const FVector2 RenderComponent2::UNIT_UPPER_LEFT = FVector2(-1.0F, 1.0F);
const FVector2 RenderComponent2::UNIT_LOWER_LEFT = FVector2(-1.0F, -1.0F);
const FVector2 RenderComponent2::UNIT_LOWER_RIGHT = FVector2(1.0F, -1.0F);

FVector2* Internal::Storage::GameObjects2::batchVertices = nullptr;
FVector2* Internal::Storage::GameObjects2::batchTexCoords = nullptr;
FColor* Internal::Storage::GameObjects2::batchColors = nullptr;
Mesh Internal::Storage::GameObjects2::batchMesh;
unsigned short Internal::Storage::GameObjects2::batchCapacity = 0;
Texture2DQuadAtlas Internal::Storage::GameObjects2::texture;
UIComponent* Internal::Storage::GameObjects2::underMouse = nullptr;
UIComponent* Internal::Storage::GameObjects2::selected = nullptr;
FMatrix3x3 Internal::Storage::GameObjects2::activeCameraMatrix = FMatrix3x3(1.0F);

Shader Internal::Storage::GameObjects2::shader;
Shader Internal::Storage::GameObjects2::independentShader;

namespace
{
	inline FMatrix3x3 ConstructCameraMatrix(const Transform2& transform)
	{
		FMatrix3x3 matrix(1.0F);

		FMatrix3x3 temp(1.0F);
		temp[0][0] = transform.GetScale().x;
		temp[1][1] = transform.GetScale().y;
		matrix *= temp;

		temp = FLOAT_MAT3_IDENTITY;
		temp[2][0] = -transform.GetPosition().x;
		temp[2][1] = -transform.GetPosition().y;
		matrix *= temp;

		return matrix;
	}
}

void Camera2::CPUStart()
{
	Internal::Storage::GameObjects2::activeCameraMatrix = ConstructCameraMatrix(gameObject->transform);
}

void Camera2::CPUStop()
{
	Internal::Storage::GameObjects2::activeCameraMatrix = FMatrix3x3(1.0F);
}

void Camera2::GPUStart()
{
	Shader::LoadMatrix3x3(0, ConstructCameraMatrix(gameObject->transform));
}

void Camera2::GPUStop()
{
	Shader::LoadMatrix3x3(0, FMatrix3x3(1.0F));
}

namespace
{
	struct ListRequest
	{
		UIComponent* object;
		bool isAdd;

		ListRequest(UIComponent* object, bool isAdd)
		{
			this->object = object;
			this->isAdd = isAdd;
		}
	};

	std::list<UIComponent*> activeUI;
	std::list<ListRequest> activeUIRequests;

	std::list<RenderComponent2*> rendering;
	std::list<IndependentRenderComponent2*> independentRendering;
}

RenderComponent2::RenderComponent2(const std::string& textureName, const FColor& color, unsigned short quadCount) :
	color(color),
	quadCount(quadCount),
	specialState(nullptr)
{
	SetTexture(textureName);
}

void RenderComponent2::SetTexture(const std::string& textureName)
{
	atlasTexInfo = Internal::Storage::GameObjects2::texture.GetTexInfo(textureName);
}

void RenderComponent2::LoadData(FVector2* vertices, FVector2* texCoords, FColor* colors)
{
	const FMatrix3x3& matrix = gameObject->transform.GetMatrix();
	vertices[0] = Transform(UNIT_UPPER_RIGHT, matrix);
	vertices[1] = Transform(UNIT_UPPER_LEFT, matrix);
	vertices[2] = Transform(UNIT_LOWER_LEFT, matrix);
	vertices[3] = Transform(UNIT_LOWER_RIGHT, matrix);

	FVector4 compositeTexInfo(
		atlasTexInfo.x + gameObject->transform.GetTexInfo().x * atlasTexInfo.z,
		atlasTexInfo.y + gameObject->transform.GetTexInfo().y * atlasTexInfo.w,
		atlasTexInfo.z * gameObject->transform.GetTexInfo().z,
		atlasTexInfo.w * gameObject->transform.GetTexInfo().w
	);
	//std::cerr << glm::to_string(compositeTexInfo) << std::endl;
	texCoords[0].x = compositeTexInfo.x + compositeTexInfo.z;
	texCoords[0].y = compositeTexInfo.y + compositeTexInfo.w;
	texCoords[1].x = compositeTexInfo.x;
	texCoords[1].y = compositeTexInfo.y + compositeTexInfo.w;
	texCoords[2].x = compositeTexInfo.x;
	texCoords[2].y = compositeTexInfo.y;
	texCoords[3].x = compositeTexInfo.x + compositeTexInfo.z;
	texCoords[3].y = compositeTexInfo.y;

	colors[0] = color;
	colors[1] = color;
	colors[2] = color;
	colors[3] = color;
}

void RenderComponent2::OnEnable()
{
	rendering.push_back(this);
	
	if (specialState == nullptr && gameObject->transform.GetParent() != nullptr && gameObject->transform.GetParent()->GetOwner() != nullptr)
		if (RenderComponent2* parentRenderComponent = gameObject->transform.GetParent()->GetOwner()->GetComponent<RenderComponent2>())
			specialState = parentRenderComponent->specialState;
}

void RenderComponent2::OnDisable()
{
	rendering.remove(this);
}

void IndependentRenderComponent2::OnEnable()
{
	independentRendering.push_back(this);
}

void IndependentRenderComponent2::OnDisable()
{
	independentRendering.remove(this);
}

bool UIComponent::ContainsPoint(float x, float y)
{
	FVector3 min = gameObject->transform.GetMatrix() * FVector3(-1.0F, -1.0F, 1.0F);
	FVector3 max = gameObject->transform.GetMatrix() * FVector3(1.0F, 1.0F, 1.0F);
	return x >= min.x && x < max.x && y >= min.y && y < max.y;
}

void UIComponent::OnEnable()
{
	activeUIRequests.push_back(ListRequest(this, true));
}

void UIComponent::OnDisable()
{
	activeUIRequests.push_back(ListRequest(this, false));

	if (Internal::Storage::GameObjects2::selected == this)
	{
		OnDeselect();
		Internal::Storage::GameObjects2::selected = nullptr;
	}
}

/*void UINode::Render()
{
	texture.Bind(0);
	Shader::LoadMatrix3x3(0, transform.GetMatrix());
	Shader::LoadVector4(1, transform.GetTexInfo());
	Shader::LoadVector4(3, &color.r);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}*/

namespace Internal
{
	namespace GameObjects2
	{
		void Init(unsigned int initialAtlasSize, bool pixelArt)
		{
			Storage::GameObjects2::shader = Shader("Internal/Shaders/2d");
			Storage::GameObjects2::shader.Bind();
			Shader::LoadMatrix3x3(0, FMatrix3x3(1.0F));

			Storage::GameObjects2::independentShader = Shader("Internal/Shaders/independent2d");
			Storage::GameObjects2::independentShader.Bind();

			Storage::GameObjects2::texture = Texture2DQuadAtlas(GL_RGBA, initialAtlasSize, !pixelArt);

			/*Storage::FGameObjects2::batchCapacity = initialBatchCapacity;
			Storage::FGameObjects2::batchVertices = new FVector2[Storage::FGameObjects2::batchCapacity * 4];
			Storage::FGameObjects2::batchTexCoords = new FVector2[Storage::FGameObjects2::batchCapacity * 4];
			Storage::FGameObjects2::batchColors = new FColor[Storage::FGameObjects2::batchCapacity * 4];

			unsigned short* batchIndices = new unsigned short[Storage::FGameObjects2::batchCapacity * 6];
			unsigned int vertexIndex = 0;
			for (unsigned int i = 0; i < Storage::FGameObjects2::batchCapacity; i += 6)
			{
				batchIndices[i] = vertexIndex;
				batchIndices[i + 1] = vertexIndex + 1;
				batchIndices[i + 2] = vertexIndex + 2;
				batchIndices[i + 3] = vertexIndex;
				batchIndices[i + 4] = vertexIndex + 2;
				batchIndices[i + 5] = vertexIndex + 3;
				vertexIndex += 4;
			}

			Storage::FGameObjects2::batchMesh = Mesh(Mesh::Descriptor(Storage::FGameObjects2::batchCapacity * 4).AddArray(Storage::FGameObjects2::batchVertices, false).AddArray(Storage::FGameObjects2::batchTexCoords, false).AddArray(Storage::FGameObjects2::batchColors, false).AddIndices(batchIndices, Storage::FGameObjects2::batchCapacity * 6));

			delete[] batchIndices;*/
		}

		void Clear()
		{
			if (Storage::GameObjects2::batchVertices != nullptr)
			{
				delete[] Storage::GameObjects2::batchVertices;
				delete[] Storage::GameObjects2::batchTexCoords;
				delete[] Storage::GameObjects2::batchColors;
			}
		}

		void Update(const GameTime& time)
		{
			for (ListRequest& request : activeUIRequests)
			{
				if (request.isAdd)
					activeUI.push_back(request.object);
				else
					activeUI.remove(request.object);
			}
			activeUIRequests.clear();

			Internal::Storage::GameObjects2::underMouse = nullptr;
			// Need to bind special state
			float mx = ((EventsState::mouseX / static_cast<float>(EventsState::windowWidth)) * 2.0F - 1.0F);
			float my = ((1.0F - EventsState::mouseY / static_cast<float>(EventsState::windowHeight)) * 2.0F - 1.0F);
			float localizedMX = mx;
			float localizedMY = my;
			SpecialState* activeSpecialState = nullptr;
			for (auto it = activeUI.rbegin(); it != activeUI.rend(); ++it)
			{
				UIComponent& node = **it;
				if (RenderComponent2* renderComponent = node.gameObject->GetComponent<RenderComponent2>())
					if (renderComponent->specialState != activeSpecialState)
					{
						if (activeSpecialState != nullptr)
							activeSpecialState->CPUStop();
						
						activeSpecialState = renderComponent->specialState;

						if (activeSpecialState != nullptr)
							activeSpecialState->CPUStart();

						FMatrix3x3 inverseCameraMatrix = glm::inverse(Internal::Storage::GameObjects2::activeCameraMatrix);
						FVector3 localizedMouseCoords = inverseCameraMatrix * FVector3(mx, my, 1.0F);
						localizedMX = localizedMouseCoords.x;
						localizedMY = localizedMouseCoords.y;
					}
				if (node.ContainsPoint(localizedMX, localizedMY))
				{
					node.OnHover(time);
					if (Input::WasButtonPressed(Input::LEFT_MOUSE_BUTTON))
						node.OnClick(Input::LEFT_MOUSE_BUTTON);
					if (Input::WasButtonPressed(Input::MIDDLE_MOUSE_BUTTON))
						node.OnClick(Input::MIDDLE_MOUSE_BUTTON);
					if (Input::WasButtonPressed(Input::RIGHT_MOUSE_BUTTON))
						node.OnClick(Input::RIGHT_MOUSE_BUTTON);

					if (Input::WasButtonReleased(Input::LEFT_MOUSE_BUTTON))
						node.OnRelease(Input::LEFT_MOUSE_BUTTON);
					if (Input::WasButtonReleased(Input::MIDDLE_MOUSE_BUTTON))
						node.OnRelease(Input::MIDDLE_MOUSE_BUTTON);
					if (Input::WasButtonReleased(Input::RIGHT_MOUSE_BUTTON))
						node.OnRelease(Input::RIGHT_MOUSE_BUTTON);

					if (Internal::Storage::GameObjects2::selected != &node && (Input::WasButtonPressed(Input::LEFT_MOUSE_BUTTON) || Input::WasButtonPressed(Input::MIDDLE_MOUSE_BUTTON) || Input::WasButtonPressed(Input::RIGHT_MOUSE_BUTTON)))
					{
						if (Internal::Storage::GameObjects2::selected != nullptr)
							Internal::Storage::GameObjects2::selected->OnDeselect();

						Internal::Storage::GameObjects2::selected = &node;
						Internal::Storage::GameObjects2::selected->OnSelect();
					}

					Internal::Storage::GameObjects2::underMouse = *it;

					return;
				}
			}

			if (Input::WasButtonPressed(Input::LEFT_MOUSE_BUTTON) || Input::WasButtonPressed(Input::MIDDLE_MOUSE_BUTTON) || Input::WasButtonPressed(Input::RIGHT_MOUSE_BUTTON))
			{
				if (Internal::Storage::GameObjects2::selected != nullptr)
					Internal::Storage::GameObjects2::selected->OnDeselect();

				Internal::Storage::GameObjects2::selected = nullptr;
			}
		}

		void Render()
		{
			glDisable(GL_CULL_FACE);
			glDisable(GL_DEPTH_TEST);
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			Storage::GameObjects2::independentShader.Bind();
			Storage::Rendering::quadMesh.Bind();
			for (IndependentRenderComponent2* component : independentRendering)
			{
				component->texture.Bind(0);
				Shader::LoadMatrix3x3(0, component->gameObject->transform.GetMatrix());
				Shader::LoadVector4(1, &component->color.r);
				Storage::Rendering::quadMesh.DrawArrays(GL_TRIANGLE_STRIP);
			}

			if (rendering.empty())
				return;

			Storage::GameObjects2::shader.Bind();

			auto nextLoadComponent = rendering.begin();
			SpecialState* specialState = nullptr;

		batch_start:
			auto component = nextLoadComponent;
			bool isLoading = true;
			unsigned short nextLoadIndex = 0;
			unsigned short index = 0;
			bool needsReturn = false;
			while (component != rendering.end())
			{
				if (isLoading)
				{
					if ((*nextLoadComponent)->specialState != specialState)
					{
						if (nextLoadIndex != 0)
						{
							Internal::Storage::GameObjects2::batchMesh.Bind();
							Internal::Storage::GameObjects2::batchMesh.Substitute(0, Storage::GameObjects2::batchVertices, 0, nextLoadIndex * 4);
							Internal::Storage::GameObjects2::batchMesh.Substitute(1, Storage::GameObjects2::batchTexCoords, 0, nextLoadIndex * 4);
							Internal::Storage::GameObjects2::batchMesh.Substitute(2, Storage::GameObjects2::batchColors, 0, nextLoadIndex * 4);

							Internal::Storage::GameObjects2::texture.Bind(0);
							Internal::Storage::GameObjects2::batchMesh.SetDrawCount(nextLoadIndex * 6);
							Internal::Storage::GameObjects2::batchMesh.DrawElements();
						}

						if (specialState != nullptr)
							specialState->GPUStop();
						specialState = (*nextLoadComponent)->specialState;
						if (specialState != nullptr)
							specialState->GPUStart();

						goto batch_start;
					}
					if ((*nextLoadComponent)->quadCount + nextLoadIndex <= Storage::GameObjects2::batchCapacity)
					{
						(*nextLoadComponent)->LoadData(Storage::GameObjects2::batchVertices + nextLoadIndex * 4, Storage::GameObjects2::batchTexCoords + nextLoadIndex * 4, Storage::GameObjects2::batchColors + nextLoadIndex * 4);
						nextLoadIndex += (*nextLoadComponent)->quadCount;
						++nextLoadComponent;
					}
					else
					{
						isLoading = false;
					}
				}
				else
				{
					if ((*component)->specialState != specialState)
					{
						//std::cerr << "found special state while counting" << std::endl;

						needsReturn = true;
						goto mesh_build;
					}
				}

				index += (*component)->quadCount;
				++component;
			}

			if (!isLoading)
			{
			mesh_build:
				unsigned short oldCapacity = Storage::GameObjects2::batchCapacity;
				FVector2* oldVertices = Storage::GameObjects2::batchVertices;
				FVector2* oldTexCoords = Storage::GameObjects2::batchTexCoords;
				FColor* oldColors = Storage::GameObjects2::batchColors;

				Storage::GameObjects2::batchCapacity = index;
				Storage::GameObjects2::batchVertices = new FVector2[Storage::GameObjects2::batchCapacity * 4];
				Storage::GameObjects2::batchTexCoords = new FVector2[Storage::GameObjects2::batchCapacity * 4];
				Storage::GameObjects2::batchColors = new FColor[Storage::GameObjects2::batchCapacity * 4];
				
				if (oldCapacity != 0)
				{
					memcpy(Storage::GameObjects2::batchVertices, oldVertices, oldCapacity * 4 * sizeof(FVector2));
					memcpy(Storage::GameObjects2::batchTexCoords, oldTexCoords, oldCapacity * 4 * sizeof(FVector2));
					memcpy(Storage::GameObjects2::batchColors, oldColors, oldCapacity * 4 * sizeof(FColor));
				}

				delete[] oldVertices;
				delete[] oldTexCoords;
				delete[] oldColors;

				while (nextLoadComponent != component)
				{
					(*nextLoadComponent)->LoadData(Storage::GameObjects2::batchVertices + nextLoadIndex * 4, Storage::GameObjects2::batchTexCoords + nextLoadIndex * 4, Storage::GameObjects2::batchColors + nextLoadIndex * 4);
					nextLoadIndex += (*nextLoadComponent)->quadCount;
					++nextLoadComponent;
				}

				unsigned int indexCount = Storage::GameObjects2::batchCapacity * 6;
				unsigned short* batchIndices = new unsigned short[indexCount];
				unsigned int vertexIndex = 0;
				for (unsigned int i = 0; i < indexCount; i += 6)
				{
					batchIndices[i    ] = vertexIndex + 0;
					batchIndices[i + 1] = vertexIndex + 1;
					batchIndices[i + 2] = vertexIndex + 2;
					batchIndices[i + 3] = vertexIndex + 0;
					batchIndices[i + 4] = vertexIndex + 2;
					batchIndices[i + 5] = vertexIndex + 3;
					vertexIndex += 4;
				}

				Storage::GameObjects2::batchMesh = Mesh(Mesh::Descriptor(Storage::GameObjects2::batchCapacity * 4).AddArray(Storage::GameObjects2::batchVertices, false).AddArray(Storage::GameObjects2::batchTexCoords, false).AddArray(Storage::GameObjects2::batchColors, false).AddIndices(batchIndices, Storage::GameObjects2::batchCapacity * 6));

				delete[] batchIndices;

				Internal::Storage::GameObjects2::batchMesh.Bind();
			}
			else
			{
				Internal::Storage::GameObjects2::batchMesh.Bind();
				Internal::Storage::GameObjects2::batchMesh.Substitute(0, Storage::GameObjects2::batchVertices, 0, nextLoadIndex * 4);
				Internal::Storage::GameObjects2::batchMesh.Substitute(1, Storage::GameObjects2::batchTexCoords, 0, nextLoadIndex * 4);
				Internal::Storage::GameObjects2::batchMesh.Substitute(2, Storage::GameObjects2::batchColors, 0, nextLoadIndex * 4);
			}
			
			Internal::Storage::GameObjects2::texture.Bind(0);
			Internal::Storage::GameObjects2::batchMesh.SetDrawCount(nextLoadIndex * 6);
			Internal::Storage::GameObjects2::batchMesh.DrawElements();

			if (needsReturn)
			{
				if (specialState != nullptr)
					specialState->GPUStop();
				specialState = (*nextLoadComponent)->specialState;
				if (specialState != nullptr)
					specialState->GPUStart();

				goto batch_start;
			}
		}
	}
}

void MouseReleasingComponent2::OnEnable()
{
	EventsState::AddCursorFreeLock();
}

void MouseReleasingComponent2::OnDisable()
{
	EventsState::FreeCursorFreeLock();
}