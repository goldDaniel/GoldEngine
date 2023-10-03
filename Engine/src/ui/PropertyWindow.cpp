#include "PropertyWindow.h"

#include <imgui.h>

#include "scene/BaseComponents.h"

static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue = 0.0f, float columnWidth = 100.0f);

PropertyWindow::PropertyWindow(scene::Scene& scene, std::function<scene::GameObject(void)>&& selectedFunc)
	: ImGuiWindow("Properties", true)
	, mScene(scene)
	, mGetSelected(std::move(selectedFunc))
{
	AddComponentControl<TransformComponent>("Transform", [](auto& t)
	{
		DrawVec3Control("Position", t.position);

		DrawVec3Control("Rotation", t.rotation);

		DrawVec3Control("Scale", t.scale, 1.0f);
		t.scale = glm::max(t.scale, { 0,0,0 });
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
	if (ImGui::Button("Add Component..."))
	{
		// TODO (danielg): component pop-up window
	}

	for (auto&& draw : mDrawCallbacks)
	{
		draw(obj);
	}
}


static void DrawVec3Control(const std::string& label, glm::vec3& values, float resetValue, float columnWidth)
{
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
	if (ImGui::Button("X", buttonSize))
	{
		values.x = resetValue;
	}

	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##X", &values.x, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushItemWidth(width);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.3f, 0.8f, 0.3f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.2f, 0.7f, 0.2f, 1.0f });
	ImGui::PushFont(font);
	if (ImGui::Button("Y", buttonSize))
	{
		values.y = resetValue;
	}
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##Y", &values.y, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();
	ImGui::SameLine();

	ImGui::PushItemWidth(width);
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4{ 0.2f, 0.35f, 0.9f, 1.0f });
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4{ 0.1f, 0.25f, 0.8f, 1.0f });
	ImGui::PushFont(font);
	if (ImGui::Button("Z", buttonSize))
	{
		values.z = resetValue;
	}
	ImGui::PopFont();
	ImGui::PopStyleColor(3);

	ImGui::SameLine();
	ImGui::DragFloat("##Z", &values.z, 0.1f, 0.0f, 0.0f, "%.2f");
	ImGui::PopItemWidth();

	ImGui::PopStyleVar();

	ImGui::Columns(1);

	ImGui::PopID();
}