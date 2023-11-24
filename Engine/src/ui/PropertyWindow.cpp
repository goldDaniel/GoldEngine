#include "PropertyWindow.h"

#include <imgui.h>

#include "scene/BaseComponents.h"

static bool DrawVec3Control(const std::string& label, glm::vec3& values, bool isColor = false, float resetValue = 0.0f, float columnWidth = 100.0f);

PropertyWindow::PropertyWindow(scene::Scene& scene, std::function<scene::GameObject(void)>&& selectedFunc)
	: ImGuiWindow("Properties", true)
	, mScene(scene)
	, mGetSelected(std::move(selectedFunc))
{
	AddComponentControl<TransformComponent>("Transform", [this](auto& t)
	{
		
		bool updated = DrawVec3Control("Position", t.position);

		DrawVec3Control("Rotation", t.rotation);

		DrawVec3Control("Scale", t.scale, 1.0f);
		t.scale = glm::max(t.scale, { 0,0,0 });

		// HACK: dirty flags needed for transforms
		if (updated && mGetSelected().HasComponent<PointLightComponent>())
		{
			mGetSelected().GetComponent<PointLightComponent>().color.w = 1;
		}
	});

	AddComponentControl<ParentComponent>("Parent", [this](auto& parentID)
	{
		scene::GameObject parent = mScene.Get(parentID.parent);
		std::string tag = parent.GetComponent<TagComponent>().tag + '(' + std::to_string((uint32_t)parentID.parent) + ')';
		ImGui::Text(tag.c_str());
	});

	AddComponentControl<ChildrenComponent>("Children", [this](auto& c)
	{
		const auto& children = c.children;
		for (const auto id : children)
		{
			auto child = mScene.Get(id);
			std::string tag = child.GetComponent<TagComponent>().tag + '(' + std::to_string((uint32_t)id) + ')';
			ImGui::Text(tag.c_str());
		}
	});

	AddComponentControl<DirectionalLightComponent>("Directional Light", [this](auto& l)
	{
		static glm::vec3 direction;
		direction = glm::vec3{ l.direction.x, l.direction.y, l.direction.z };
		DrawVec3Control("Direction", direction);

		glm::vec3 color = l.color;
		DrawVec3Control("Color", color, true);
		
		// set the w component to 1 marks the light dirty, needing to be updated on GPU 
		if (direction != glm::vec3{ l.direction.x, l.direction.y, l.direction.z })
		{
			direction = glm::clamp(direction, { -1, -1, -1 }, { 1,1,1 });
			if (glm::length2(direction) > 0)
			{
				l.direction = glm::vec4(direction, 1.0f);
			}
			
		}

		// set the w component to 1 marks the light dirty, needing to be updated on GPU 
		if (color != glm::vec3{ l.color.r, l.color.g, l.color.b })
		{	
			color = glm::max({ 0,0,0 }, color);

			l.color = { color, 1 };
			l.direction.w = 1;
		}
	});

	AddComponentControl<PointLightComponent>("Point Light", [this](auto& l)
	{
		glm::vec3 color = l.color;
		DrawVec3Control("Color", color, true);

		// set the w component to 1 marks the light dirty, needing to be updated on GPU 
		if (color != glm::vec3{ l.color.r, l.color.g, l.color.b })
		{
			color = glm::max({ 0,0,0 }, color);

			l.color = { color, 1 };
		}

		ImGui::Text("Falloff");
		ImGui::SameLine();
		if (ImGui::SliderFloat("Falloff", &l.falloff, 0.0f, 100.f))
		{
			l.color.w = 1;
		}
	});

	AddComponentControl<ShadowMapComponent>("Shadow Map", [this](auto& shadow)
	{
		auto check = [&shadow](bool changed)
		{
			shadow.dirty = shadow.dirty || changed;
		};

		auto obj = mGetSelected();
		if (obj.HasComponent<DirectionalLightComponent>())
		{
			auto& light = obj.GetComponent<DirectionalLightComponent>();
			
			ImGui::Text("Directional Light Ortho Projection");
			
			ImGui::Text("Left"); ImGui::SameLine(); check(ImGui::SliderFloat("Left", &shadow.left, -200, 200));
			ImGui::Text("Right"); ImGui::SameLine(); check(ImGui::SliderFloat("Right", &shadow.right, -200, 200));

			ImGui::Text("Bottom"); ImGui::SameLine(); check(ImGui::SliderFloat("Bottom", &shadow.bottom, -200, 200));
			ImGui::Text("Top"); ImGui::SameLine(); check(ImGui::SliderFloat("top", &shadow.top, -200, 200));

			ImGui::Text("Near"); ImGui::SameLine(); check(ImGui::SliderFloat("Near", &shadow.nearPlane, -200, 200));
			ImGui::Text("Far"); ImGui::SameLine(); check(ImGui::SliderFloat("Far", &shadow.farPlane, -200, 200));

			ImGui::Text("Bias"); ImGui::SameLine(); check(ImGui::SliderFloat("Bias", &shadow.shadowMapBias[0], 0.0f, 0.01f));

			ImGui::Text("PCF Size"); ImGui::SameLine();  check(ImGui::SliderInt("PCF size", &shadow.PCFSize, 0, 9));
		}
		else if (obj.HasComponent<PointLightComponent>())
		{
			ImGui::Text("Point Light Perspective Projection");

			float fov = glm::degrees(shadow.FOV);
			ImGui::Text("Aspect"); ImGui::SameLine(); check(ImGui::SliderFloat("FOV", &fov, 35, 120));
			shadow.FOV = glm::radians(fov);

			ImGui::Text("Aspect"); ImGui::SameLine(); check(ImGui::SliderFloat("Aspect", &shadow.aspect, 0.1, 2));

			ImGui::Text("Near"); ImGui::SameLine(); check(ImGui::SliderFloat("Near", &shadow.nearPlane, 0.5f, 10));

			ImGui::Text("Far"); ImGui::SameLine(); check(ImGui::SliderFloat("Far", &shadow.farPlane, 50, 5000));

			ImGui::Text("Bias"); ImGui::SameLine(); check(ImGui::SliderFloat("Bias", &shadow.shadowMapBias[0], 0.0f, 0.01f));
			
			// gross
			shadow.shadowMapBias[1] = shadow.shadowMapBias[2] = shadow.shadowMapBias[3] =
			shadow.shadowMapBias[4] = shadow.shadowMapBias[5] = shadow.shadowMapBias[0];

			ImGui::Text("PCF Size");
			ImGui::SameLine();
			check(ImGui::SliderInt("PCF size", &shadow.PCFSize, 0, 9));
			if (shadow.dirty)
			{
				//HACK: using as a flag to update shadowmaps temporarily
				mScene.ForEach<LightBufferComponent>([](scene::GameObject obj)
				{
					auto& buffer = obj.GetComponent<LightBufferComponent>();
					buffer.isDirty = false;
				});
			}
		}
		else
		{
			DEBUG_ASSERT(false, "ShadowMap with no light!?");
		}
	});

	AddComponentControl<RenderComponent>("Render", [this](auto& render)
	{
		/*uint32_t materialID = render.material;
		uint32_t meshID = render.mesh;*/

		// auto materialService = core::Singletons::Get()->Resolve<MaterialResourceService>();
		// auto meshService = core::Singletons::Get()->Resolve<MeshResourceService>();

		// TODO (danielg): draw mesh
		// TODO (danielg): draw material
	});
}


template<typename T, typename DrawFunc>
void PropertyWindow::DrawComponent(const std::string& name, scene::GameObject object, DrawFunc func)
{
	if(object.HasComponent<T>())
	{
		const ImGuiTreeNodeFlags flags =	ImGuiTreeNodeFlags_DefaultOpen |
											ImGuiTreeNodeFlags_Framed |
											ImGuiTreeNodeFlags_SpanAvailWidth |
											ImGuiTreeNodeFlags_AllowItemOverlap |
											ImGuiTreeNodeFlags_FramePadding;

		ImGuiIO& io = ImGui::GetIO();
		auto font = io.Fonts->Fonts[0];

		auto& comp = object.GetComponent<T>();
		ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		float lineHeight = font->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;

		ImGui::Separator();

		bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), flags, name.c_str());

		ImGui::PopStyleVar();
		ImGui::SameLine(contentRegionAvailable.x - lineHeight * 0.5f);

		if (ImGui::Button("+", ImVec2{ lineHeight, lineHeight }))
		{
			ImGui::OpenPopup("ComponentSettings");
		}

		bool removeComponent = false;
		if (ImGui::BeginPopup("ComponentSettings"))
		{
			if (ImGui::MenuItem("Remove component"))
			{
				removeComponent = true;
			}
			ImGui::EndPopup();
		}

		if (open)
		{
			func(comp);
			ImGui::TreePop();
		}

		if (removeComponent)
		{
			object.RemoveComponent<T>();
		}
	}
}

void PropertyWindow::DrawWindow(graphics::Renderer& renderer, gold::ServerResources& resources)
{
	scene::GameObject obj = mGetSelected();

	if (!obj.IsValid()) return;

	if (obj.HasComponent<TagComponent>())
	{
		std::string& tag = obj.GetComponent<TagComponent>().tag;

		char buffer[256];
		memset(buffer, 0, sizeof(buffer));
		strncpy_s(buffer, sizeof(buffer), tag.c_str(), sizeof(buffer));
		if (ImGui::InputText("##Tag", buffer, sizeof(buffer)))
		{
			tag = std::string(buffer);
		}
	}

	ImGui::SameLine();
	ImGui::PushItemWidth(-1);
	if (ImGui::Button("Add Component"))
	{
		// TODO (danielg): component pop-up window
	}

	for (auto&& draw : mDrawCallbacks)
	{
		draw(obj);
	}
}


static bool DrawVec3Control(const std::string& label, glm::vec3& values, bool isColor, float resetValue, float columnWidth)
{
	bool result = false;
	ImGuiIO& io = ImGui::GetIO();
	auto font = io.Fonts->Fonts[0];

	ImGui::PushID(label.c_str());

	ImGui::Columns(2);
	ImGui::SetColumnWidth(0, columnWidth);
	ImGui::Text(label.c_str());
	ImGui::NextColumn();

	float width = ImGui::CalcItemWidth() / 3;
	ImGui::PushItemWidth(width);
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });


	float lineHeight = font->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f;
	ImVec2 buttonSize = { lineHeight + 3.0f, lineHeight };

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.9f, 0.2f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.8f, 0.1f, 0.15f, 1.0f });
	ImGui::PushFont(font);
	if (ImGui::Button(isColor ? "R" : "X", buttonSize))
	{
		result = true;
		values.x = resetValue;
	}

	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	result = result || ImGui::DragFloat(isColor ? "##R" : "##X", &values.x, 0.01f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushItemWidth(width);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushFont(font);
	if (ImGui::Button(isColor ? "G" : "Y", buttonSize))
	{
		result = true;
		values.y = resetValue;
	}
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	result = result || ImGui::DragFloat(isColor ? "##G" : "##Y", &values.y, 0.01f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushItemWidth(width);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushFont(font);
	if (ImGui::Button(isColor ? "B" : "Z", buttonSize))
	{
		result = true;
		values.z = resetValue;
	}
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	result = result || ImGui::DragFloat(isColor ? "##B" : "##Z", &values.z, 0.01f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();

	return result;
}