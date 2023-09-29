#include "SceneWindow.h"


#include "scene/BaseComponents.h"
#include <imgui.h>

SceneWindow::SceneWindow(scene::Scene* scene)
	: ImGuiWindow("Scene", true)
	, mScene(scene)
{
}

void SceneWindow::SetSelected(scene::GameObject obj)
{
	mSelected = obj;
}

scene::GameObject SceneWindow::GetSelected() const
{
	return mSelected;
}

void SceneWindow::DrawWindow()
{
	if (!mScene) return; 
	

	if (ImGui::Button("Add GameObject"))
	{
		mSelected = mScene->CreateGameObject();
	}

	mScene->ForEach([this](scene::GameObject& obj)
	{
		if (!obj.HasParent())
		{
			_DrawNode(obj);
		}
	});
}

void SceneWindow::_DrawNode(scene::GameObject obj)
{
	auto tag = obj.GetComponent<TagComponent>().tag + '(' + std::to_string((uint32_t)obj.GetEntity()) + ')';

	ImGuiTreeNodeFlags flags = ((mSelected == obj) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_OpenOnArrow;
	flags |= ImGuiTreeNodeFlags_SpanAvailWidth;
	bool opened = ImGui::TreeNodeEx((void*)(uint64_t)obj.GetEntity(), flags, tag.c_str());

	if (ImGui::IsItemClicked())
	{
		mSelected = obj;
	}

	bool deleteObj = false;
	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Delete GameObject"))
		{
			deleteObj = true;
		}
		if (ImGui::MenuItem("Add Child GameObject"))
		{
			auto obj = mScene->CreateGameObject("Child");
			obj.SetParent(mSelected);
		}

		ImGui::EndPopup();
	}

	if (opened)
	{
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		
		obj.ForEachChild([this](scene::GameObject& child)
		{
			_DrawNode(child);
		});
		
		ImGui::TreePop();
	}

	if (deleteObj)
	{
		mScene->DestroyGameObject(obj);
		if (mSelected == obj)
		{
			mSelected = scene::GameObject();
		}
	}
}