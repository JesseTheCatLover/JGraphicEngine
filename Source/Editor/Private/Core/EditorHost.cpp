//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EditorHost.h"

#include "TPanelContainer.h"
#include "ToolService.h"
#include "Subsystems/ViewportSubsystem.h"

EditorHost::EditorHost(EditorRuntime& runtime):
m_EditorRuntime(runtime)
{
    m_ToolService = TUniquePtr<ToolService>(new ToolService());
    m_PanelContainer = TUniquePtr<TPanelContainer>(new TPanelContainer(*this, runtime, *m_ToolService));

    RegisterCoreSubsystems();
    RegisterCoreServices();
}

void EditorHost::RegisterCoreSubsystems()
{
    // Core subsystems
    m_PanelContainer->RegisterSubsystem<ViewportSubsystem>(*this, m_EditorRuntime, *m_ToolService);
    // m_PanelContainer->RegisterSubsystem<HierarchySubsystem>(*this, m_EditorRuntime, *m_ToolService);
}

void EditorHost::RegisterCoreServices()
{
//     m_Services.Register<SceneQueryService>(m_EditorRuntime);
//     m_Services.Register<SelectionService>(m_EditorRuntime);
//     m_Services.Register<HierarchyService>(*this);
//     m_Services.Register<PickingService>(*this);
}

void EditorHost::Tick(float deltaTime)
{
    m_Services->Tick(deltaTime);
    m_PanelContainer->Tick(deltaTime);
}


void EditorHost::UpdateHierarchySnapshot()
{
    // Ask EngineEditor -> EditorSceneAPI for a fresh snapshot
    m_HierarchySnapshot = m_EngineEditor.GetSceneAPI().BuildHierarchySnapshot();

    // Mark which ones are selected
    for (auto& node : m_HierarchySnapshot)
    {
        node.isSelected =
            std::find(m_SelectedActors.begin(), m_SelectedActors.end(), node.id) != m_SelectedActors.end();
    }
}


void EditorHost::PickActorAtViewportPos(const IEditorPanel* panel, float x, float y, const FSelectionModifiers& mods)
{
    // 1) Find the camera associated with this panel
    auto camIt = m_PanelToCameraMap.find(panel);
    if (camIt == m_PanelToCameraMap.end())
        return;

    UDynamicID::IDType cameraID = camIt->second;
    CameraEditorTool* cameraTool = m_EngineEditor.GetCameraEditorTool(cameraID);
    if (!cameraTool)
        return;

    // 2) Get viewport size for this panel
    auto vpIt = m_CameraStateMap.find(panel);
    if (vpIt == m_CameraStateMap.end())
        return;

    const float width  = vpIt->second.width;
    const float height = vpIt->second.height;
    if (width <= 0.f || height <= 0.f)
        return;

    // 3) Convert local pixel coords -> NDC [-1,1]
    // x,y are in [0..width],[0..height] with (0,0) at top-left of the Image.
    const float xNDC =  2.0f * (x / width)  - 1.0f;
    const float yNDC =  1.0f - 2.0f * (y / height); // convert top-left to NDC

    const float aspect = width / height;

    // 4) Common camera data
    const FQuat camRot = cameraTool->GetRotation();
    const FVector3 camPos = cameraTool->GetPosition();

    FVector3 originWorld;
    FVector3 dirWorld;

    if (cameraTool->GetProjectionType() == EProjectionType::Perspective)
    {
        // ---------------------------
        // Perspective ray
        // Left-handed, X = forward, Y = right, Z = up
        // ---------------------------
        const float verticalFovDeg = cameraTool->GetFOV();
        const float halfFovRad = verticalFovDeg * 0.5f * (3.1415926535f / 180.0f);
        const float tanHalfFov = std::tan(halfFovRad);

        // Point on a plane 1 unit in front of the camera (camera space)
        FVector3 dirCam;
        dirCam.x = 1.0f;                        // forward (X)
        dirCam.y = xNDC * tanHalfFov * aspect;  // right   (Y)
        dirCam.z = yNDC * tanHalfFov;           // up      (Z)
        dirCam   = dirCam.Normalized();

        // Rotate into world space
        dirWorld  = camRot.RotateVector(dirCam).Normalized();
        originWorld = camPos; // ray starts at camera position
    }
    else
    {
        // ---------------------------
        // Orthographic ray
        // Direction is constant (camera forward), origin slides in the view plane
        // ---------------------------

        // Ortho volume in camera space:
        //  - vertical half-size is m_OrthoHalfHeight
        //  - horizontal half-size = halfHeight * aspect
        const float halfHeight = cameraTool->GetOrthoHalfHeight();
        const float halfWidth = halfHeight * aspect;

        // NDC -> camera-space offsets on the view plane
        // In camera space: X = forward, Y = right, Z = up
        FVector3 pointCam;
        pointCam.x = 0.0f;  // on plane through camera, forward handled by dir
        pointCam.y = xNDC * halfWidth; // right offset
        pointCam.z = yNDC * halfHeight; // up offset

        // Transform this point into world
        const FVector3 pointWorld = camRot.RotateVector(pointCam) + camPos;

        // Forward direction in camera space is +X
        const FVector3 forwardCam(1.0f, 0.0f, 0.0f);
        dirWorld = camRot.RotateVector(forwardCam).Normalized();

        originWorld = pointWorld;
    }

    // 5) Build ray
    FRay ray;
    ray.origin = originWorld;
    ray.direction = dirWorld;

    // 6) Raycast
    FRaycastHit hit{};
    const bool bHit = m_EngineEditor.GetSceneAPI().Raycast(ray, hit) && hit.bHit;

    if (bHit)
    {
        HandleSelectionClick(hit.actorID, mods);
    }
    else
    {
        ClearSelection();
    }
}



// Helper: build a "visible order" list from the snapshot using parentID tree.
// Preorder traversal (parent then its children), matching your tree draw order.
static void BuildVisibleOrderRecursive(
    ActorID parent,
    const std::vector<FHierarchySnapshot>& snap,
    std::vector<ActorID>& outOrder)
{
    for (const auto& n : snap)
    {
        if (n.parentID == parent)
        {
            outOrder.push_back(n.id);
            if (n.hasChildren)
                BuildVisibleOrderRecursive(n.id, snap, outOrder);
        }
    }
}

static std::vector<ActorID> BuildVisibleOrder(const std::vector<FHierarchySnapshot>& snap)
{
    std::vector<ActorID> order;
    order.reserve(snap.size());
    BuildVisibleOrderRecursive(0, snap, order); // root actors parentID = 0
    return order;
}

void EditorHost::HandleSelectionClick(ActorID id, const FSelectionModifiers& mods)
{
    if (id == 0)
        return;

    if (mods.bRange)
        SelectRangeTo(id);
    else if (mods.bToggle)
        ToggleActorSelection(id);
    else
        SelectSingleActor(id);
}

void EditorHost::SelectSingleActor(ActorID id)
{
    m_SelectedActors.clear();

    if (id != 0)
    {
        m_SelectedActors.push_back(id);
        m_SelectionAnchor = id;
        m_RevealInHierarchy = id; // reveal request
    }
    else
    {
        m_SelectionAnchor = 0;
    }

    m_EngineEditor.GetSceneAPI().SetSelectedActors(m_SelectedActors);
}

void EditorHost::ToggleActorSelection(ActorID id)
{
    if (id == 0) return;

    auto it = std::find(m_SelectedActors.begin(), m_SelectedActors.end(), id);
    if (it == m_SelectedActors.end())
        m_SelectedActors.push_back(id);
    else
        m_SelectedActors.erase(it);

    m_SelectionAnchor = id;
    m_RevealInHierarchy = id; // last interacted item
    m_EngineEditor.GetSceneAPI().SetSelectedActors(m_SelectedActors);
}

void EditorHost::SelectRangeTo(ActorID id)
{
    if (m_HierarchySnapshot.empty())
        return;

    // No anchor yet? behave like single click.
    if (m_SelectionAnchor == 0)
    {
        SelectSingleActor(id);
        return;
    }

    const auto order = BuildVisibleOrder(m_HierarchySnapshot);

    auto itA = std::find(order.begin(), order.end(), m_SelectionAnchor);
    auto itB = std::find(order.begin(), order.end(), id);

    if (itA == order.end() || itB == order.end())
    {
        SelectSingleActor(id);
        return;
    }

    const auto begin = std::min(itA, itB);
    const auto end   = std::max(itA, itB);

    m_SelectedActors.clear();
    m_SelectedActors.reserve(static_cast<size_t>(end - begin + 1));

    for (auto it = begin; it != end + 1; ++it)
        m_SelectedActors.push_back(*it);

    // Keep anchor unchanged (matches OS explorers)
    m_EngineEditor.GetSceneAPI().SetSelectedActors(m_SelectedActors);
}

void EditorHost::ClearSelection()
{
    m_SelectedActors.clear();
    m_SelectionAnchor = 0;
    m_EngineEditor.GetSceneAPI().SetSelectedActors(m_SelectedActors);
}