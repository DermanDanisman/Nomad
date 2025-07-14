// Copyright (C) Developed by Pask, Published by Dark Tower Interactive SRL 2024. All Rights Reserved.

#include "AMSMapWidget.h"
#include "AMSActorMarker.h"
#include "AMSMapArea.h"
#include "AMSMapMarkerComponent.h"
#include "AMSMapSubsystem.h"
#include "AMSMarkerWidget.h"
#include "AMSTypes.h"
#include "ANSUIPlayerSubsystem.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/InputComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Layout/Geometry.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Widgets/SWidget.h"
#include <CommonInputSubsystem.h>
#include <Components/Widget.h>
#include <Engine/GameInstance.h>
#include <GameFramework/Pawn.h>
#include <GameFramework/PlayerController.h>
#include <Kismet/GameplayStatics.h>

UAMSMapWidget::UAMSMapWidget(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    MarkersClass = UAMSMarkerWidget::StaticClass();
    MarkersSize = FVector2D(32.f, 32.f);
    ActorMarkerClass = AAMSActorMarker::StaticClass();
    CanvasSize = FVector2D(1024.f);
    MarkerScaleWhenHighlighted = FVector2D(1.3f, 1.3f);
}

void UAMSMapWidget::ZoomIn(float zoomDelta)
{

    if (!MapBrush || !MapBrush->IsValidLowLevel())
    {
        return;
    }

    const float OldZoomLevel = CurrentZoomLevel;
    const float ZoomFactor = zoomDelta * ZoomSpeed;
    const float NewZoomLevel = FMath::Clamp(CurrentZoomLevel + ZoomFactor, MinimumZoomLevel, MaximumZoomLevel);

    const UWorld* World = GetWorld();
    ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
    if (!LocalPlayer || !World)
    {
        return;
    }

    APlayerController* PC = LocalPlayer->GetPlayerController(World);
    if (!PC || !PC->IsLocalController())
    {
        return;
    } 

    int32 ScreenX, ScreenY;
    PC->GetViewportSize(ScreenX, ScreenY);
    FVector2D ReferencePos = FVector2D(ScreenX / 2.0f, ScreenY / 2.0f);

    FGeometry Geometry = MapBrush->GetCachedGeometry();
    if (Geometry.GetLocalSize().IsNearlyZero())
    {
        return;
    }

    FVector2D LocalRefPos = Geometry.AbsoluteToLocal(ReferencePos);
    FVector2D MapCenter = Geometry.GetLocalSize() / 2.0f;

    FVector2D ZoomOffset = (LocalRefPos - MapCenter) * (NewZoomLevel / OldZoomLevel - 1.0f);

    SetCurrentZoomLevel(NewZoomLevel);

    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapBrush->Slot))
    {
        FVector2D OldPos = CanvasSlot->GetPosition();
        FVector2D NewPos = OldPos - ZoomOffset;
        Internal_SetCanvasPosition(NewPos);
    }

    UpdateMarkers();
}



void UAMSMapWidget::ProcessKeyDown(const FKey& pressedKey)
{
    // Start zooming
    if (pressedKey == ZoomInKey) {
        currentZoomState = EZoomState::EZoomIn;
    }
    else if (pressedKey == ZoomOutKey) {
        currentZoomState = EZoomState::EZoomOut;
    }
    // Close map
    else if (pressedKey == RemoveFromParentKey) {
        if (auto UISub = GetGameInstance()->GetSubsystem<UANSUIPlayerSubsystem>()) {
            if (UUserWidget* Cur = UISub->GetCurrentWidget()) {
                UISub->RemoveInGameWidget(Cur, true, true);
            }
        }
    }
    // Gamepad marker spawn / track
    else if (pressedKey == SpawnActorMarkerKeyGamepad) {
        if (HasAnyHoveredMarker()) {
            TrackHoveredMarker();
        } else {
            GetMapSubsystem()->RemoveAllMarkerActors();
            SpawnMarkerActorAtScreenPosition(GetCursorPosition());
        }
    }
}

void UAMSMapWidget::ProcessKeyUp(const FKey& releasedKey)
{
    // Stop continuous zoom
    if (releasedKey == ZoomInKey || releasedKey == ZoomOutKey)
    {
        currentZoomState = EZoomState::ENone;
    }
}

FReply UAMSMapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    FKey Key = InKeyEvent.GetKey();
    if (Key != ZoomInKey || Key != ZoomOutKey || Key != RemoveFromParentKey)
    {
        // Forward unrecognized keys up to parent widget
        if (UUserWidget* Parent = Cast<UUserWidget>(GetParent()))
        {
            Parent->OnKeyDown(InGeometry, InKeyEvent);
            return FReply::Handled();
        }
    }
    
    return FReply::Unhandled();
}

void UAMSMapWidget::ProcessAnalogChange(FGeometry MyGeometry, FAnalogInputEvent InAnalog)
{
    FVector2D offset(0.f,0.f);
    bool bValid = false;

    // Build pan vector from axes
    if (InAnalog.GetKey() == MoveUpAxis)    { offset.Y = InAnalog.GetAnalogValue(); bValid = true; }
    if (InAnalog.GetKey() == MoveRightAxis) { offset.X = InAnalog.GetAnalogValue(); bValid = true; }

    if (!bValid) return;
    offset *= UGameplayStatics::GetWorldDeltaSeconds(this);

    // Determine whether to pan map or move cursor
    if (MapCursor) {
        auto cursorSlot = Cast<UCanvasPanelSlot>(MapCursor->Slot);
        FVector2D curPos = cursorSlot->GetPosition();
        FVector2D mapOff(0.f,0.f);

        // Only pan map when cursor reaches edge threshold
        if ((curPos.X > MoveMapStart.X && offset.X > 0) ||
            (curPos.X < -MoveMapStart.X && offset.X < 0)) mapOff.X = offset.X;
        if ((curPos.Y > MoveMapStart.Y && offset.Y < 0) ||
            (curPos.Y < -MoveMapStart.Y && offset.Y > 0)) mapOff.Y = offset.Y;

        if (mapOff != FVector2D::ZeroVector) {
            MoveMap(offset);
        } else {
            offset *= 2.f; // faster cursor
        }

        MoveCursor(-offset);
        FVector2D pixelPos, viewportPos;
        USlateBlueprintLibrary::AbsoluteToViewport(this,
            MapCursor->GetCachedGeometry().GetAbsolutePosition(),
            pixelPos, viewportPos);
        UGameplayStatics::GetPlayerController(this, 0)->SetMouseLocation(pixelPos.X, pixelPos.Y);
    }
}

FVector2D UAMSMapWidget::GetCursorPosition() const
{
    if (MapCursor)
    {
        return MapCursor->GetCachedGeometry().GetAbsolutePosition();
    }
    return FVector2D();
}

void UAMSMapWidget::ProcessMouseClick(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
    if (!MouseEvent.IsMouseButtonDown(SpawnActorMarkerKey))
    {
        return;
    }

    SpawnMarkerActorAtMousePosition(MouseEvent);
}

void UAMSMapWidget::SpawnMarkerActorAtMousePosition(const FPointerEvent& MouseEvent)
{
    const FVector2D screenPos = MouseEvent.GetScreenSpacePosition();
    SpawnMarkerActorAtScreenPosition(screenPos);
}

void UAMSMapWidget::SpawnMarkerActorAtScreenPosition(const FVector2D screenPos)
{
    if (MapBrush)
    {

        const FVector2D widgetPos = USlateBlueprintLibrary::AbsoluteToLocal(MapBrush->GetCachedGeometry(), screenPos);
        const FVector2D normalizedPos = widgetPos / GetMapSize();

        AAMSMapArea* mapArea = GetMapArea();
        if (mapArea)
        {
            mapArea->SpawnActorMarkerAtMapLocation(normalizedPos, ActorMarkerClass);

            HandleMarkerActorsChanged();
            OnMarkerActorsChanged.Broadcast();
        }
    }
}

FVector UAMSMapWidget::GetWorldLocationFromMousePosition(const FPointerEvent& MouseEvent)
{
    const FVector2D screenPos = MouseEvent.GetScreenSpacePosition();
    const FVector2D widgetPos = USlateBlueprintLibrary::AbsoluteToLocal(MapBrush->GetCachedGeometry(), screenPos);
    const FVector2D normalizedPos = widgetPos / GetMapSize();
    return GetWorldLocationFromNormalizedMapPosition(normalizedPos);
}

FVector UAMSMapWidget::GetWorldLocationFromNormalizedMapPosition(const FVector2D& widgetPosition)
{
    const AAMSMapArea* mapArea = GetMapArea();
    if (mapArea)
    {
        return mapArea->GetWorldLocationFromNormalized2DPosition(widgetPosition);
    }
    return FVector();
}

void UAMSMapWidget::MoveRight(float RightPanAxis)
{
    MoveMap(FVector2D(0.f, RightPanAxis));
}

void UAMSMapWidget::MoveUp(float UpAxis)
{
    MoveMap(FVector2D(0.f, UpAxis));
}

void UAMSMapWidget::MoveMap(const FVector2D& Offset)
{
    const FVector2D finalDelta = Offset * MoveSpeed;
    MoveMapByPixelOffset(finalDelta);
}

void UAMSMapWidget::MoveMapByPixelOffset(const FVector2D& finalDelta)
{
    if (MapBrush && MapBrush->IsValidLowLevel())
    {
        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapBrush->Slot);
        if (CanvasSlot)
        {
            const FVector2D CanvasPos = CanvasSlot->GetPosition();
            FVector2D finalPos;
            finalPos.X = CanvasPos.X - finalDelta.X;
            finalPos.Y = CanvasPos.Y + finalDelta.Y;
            Internal_SetCanvasPosition(finalPos);
        }
    }
}

void UAMSMapWidget::MoveCursor(const FVector2D& Offset)
{
    const FVector2D finalDelta = Offset * MoveCursorSpeed;
    if (MapCursor)
    {
        UCanvasPanelSlot* cursorSlot = Cast<UCanvasPanelSlot>(MapCursor->Slot);
        if (cursorSlot)
        {
            const FVector2D CanvasPos = cursorSlot->GetPosition();
            FVector2D updatedPos;
            updatedPos.X = CanvasPos.X - finalDelta.X;
            updatedPos.Y = CanvasPos.Y + finalDelta.Y;

            FVector2D finalPos = updatedPos;
            finalPos.Y = FMath::Clamp(updatedPos.Y, -CursorLimit.Y, CursorLimit.Y);
            finalPos.X = FMath::Clamp(updatedPos.X, -CursorLimit.X, CursorLimit.X);
            cursorSlot->SetPosition(finalPos);
        }
    }
}

void UAMSMapWidget::Internal_SetCanvasPosition(const FVector2D& updatedPos)
{
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapBrush->Slot);

    const FVector2D CanvasPos = CanvasSlot->GetPosition();

    const FVector2D MaxPos = (CanvasSlot->GetSize() / 2 - CanvasSize / 2);
    const FVector2D MinPos = -(CanvasSlot->GetSize() / 2 - CanvasSize / 2);

    FVector2D finalPos = updatedPos;
    finalPos.Y = FMath::Clamp(updatedPos.Y, MinPos.Y, MaxPos.Y);
    finalPos.X = FMath::Clamp(updatedPos.X, MinPos.X, MaxPos.X);
    CanvasSlot->SetPosition(finalPos);
}

void UAMSMapWidget::CenterOnLocalPlayer()
{
    const APawn* localPawn = UGameplayStatics::GetPlayerPawn(this, 0);
    if (localPawn)
    {
        const FVector actorLoc = localPawn->GetActorLocation();
        CenterOnWorldLocation(actorLoc);
    }
}

void UAMSMapWidget::CenterOnWorldLocation(const FVector& actorLoc)
{
    const AAMSMapArea* areaMap = GetMapArea();
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapBrush->Slot);
    if (areaMap && CanvasSlot)
    {
        const FVector2D normalizedPlayer = areaMap->GetNormalized2DPositionFromWorldLocation(actorLoc);
        const FVector2D playerpos = .5 * GetMapSize() - (normalizedPlayer * GetMapSize());
        Internal_SetCanvasPosition(playerpos);
    }
}

void UAMSMapWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    InitCanvas();
    
}

void UAMSMapWidget::NativeConstruct()
{
    Super::NativeConstruct();

    UAMSMapSubsystem* mapSubsystem = GetMapSubsystem();
    if (mapSubsystem)
    {
        mapSubsystem->OnMapMarkerAdded.AddDynamic(this, &UAMSMapWidget::HandleMarkerAdded);
        mapSubsystem->OnMapMarkerRemoved.AddDynamic(this, &UAMSMapWidget::HandleMarkerRemoved);
        mapSubsystem->OnTrackedMarkerChanged.AddDynamic(this, &UAMSMapWidget::HandleTrackedMarkerChanged);
    }

    UCommonInputSubsystem* commonInputSub = GetInputSubsystem();
    if (commonInputSub)
    {
        commonInputSub->OnInputMethodChangedNative.AddUObject(this, &UAMSMapWidget::HandleInputChanged);
        HandleInputChanged(commonInputSub->GetCurrentInputType());
    }

    if (MapMask)
    {
        UMaterialInstanceDynamic* mat = MapMask->GetEffectMaterial();
        if (mat && Mask)
        {
            mat->SetTextureParameterValue("Texture", Mask);
            mat->SetTextureParameterValue("Texture2", Mask);
        } else
        {
            UE_LOG(LogTemp, Error, TEXT("Missing Mask material OR Mask Texture! - UAMSMapWidget::NativePreConstruct"));
        }
    }
    
    SetCurrentZoomLevel(DefaultZoomLevel);
    UpdateMarkers();

    bPendingTrackUpdate = true;
}

void UAMSMapWidget::HandleInputChanged(ECommonInputType bNewInputType)
{
    switch (bNewInputType)
    {
    case ECommonInputType::Gamepad:
        MapCursor->SetVisibility(ESlateVisibility::Visible);
    // UGameplayStatics::GetPlayerController(this, 0)->bShowMouseCursor = false;
        MapBrush->SetCursor(EMouseCursor::None);
        break;
    case ECommonInputType::MouseAndKeyboard:
        MapCursor->SetVisibility(ESlateVisibility::Collapsed);
    //   UGameplayStatics::GetPlayerController(this, 0)->bShowMouseCursor = true;
        MapBrush->SetCursor(EMouseCursor::Default);
        break;
    default:
        break;
    }
}

void UAMSMapWidget::NativeDestruct()
{
    Super::NativeDestruct();
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    UAMSMapSubsystem* mapSubsystem = GameInstance->GetSubsystem<UAMSMapSubsystem>();
    if (mapSubsystem)
    {
        mapSubsystem->OnMapMarkerAdded.RemoveDynamic(this, &UAMSMapWidget::HandleMarkerAdded);
        mapSubsystem->OnMapMarkerRemoved.RemoveDynamic(this, &UAMSMapWidget::HandleMarkerRemoved);
    }
    UCommonInputSubsystem* commonInputSub = GetInputSubsystem();
    if (commonInputSub)
    {
        commonInputSub->OnInputMethodChangedNative.RemoveAll(this);
    }
    /* UGameplayStatics::GetPlayerController(this, 0)->bShowMouseCursor = false;*/
    SetCurrentZoomLevel(DefaultZoomLevel);
}

void UAMSMapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    switch (currentZoomState)
    {
    case EZoomState::ENone:
        break;
    case EZoomState::EZoomIn:
        ZoomIn(InDeltaTime * 10.f );
        break;
    case EZoomState::EZoomOut:
        ZoomIn(InDeltaTime * -10.f );
        break;
    }

    if (bPendingMarkersUpdate)
    {
        Internal_UpdateMarkers();
    }

    if (bPendingTrackUpdate)
    {
        FAMSMarker markerRef = GetCurrentlytTrackedMarker();
        if (markerRef.ValidCheck())
        {
            TrackMarker(markerRef);
        }
        bPendingTrackUpdate = false;
    }
}

FAMSMarker UAMSMapWidget::GetCurrentlytTrackedMarker() const
{
    const UAMSMapMarkerComponent* markerComp = GetMapSubsystem()->GetCurrentlytTrackedMarker();
    const FAMSMarker* markerRef = markerWidgets.FindByKey(markerComp);
    if (markerRef && markerRef->ValidCheck())
    {
        return *markerRef;
    }
    return FAMSMarker();
}

void UAMSMapWidget::HandleTrackedMarkerChanged_Implementation(UAMSMapMarkerComponent* marker) {}

void UAMSMapWidget::ResetDefaultZoom()
{
    if (MapBrush && MapBrush->IsValidLowLevel())
    {

        UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapBrush->Slot);
        if (CanvasSlot)
        {
            CanvasSlot->SetAlignment(InitialAlignment);
            CanvasSlot->SetSize(InitialCanvasSize);
            SetCurrentZoomLevel(DefaultZoomLevel);
            Internal_SetCanvasPosition(InitialCanvasPosition);
        }
    }
}

void UAMSMapWidget::SetCurrentZoomLevel(float val)
{
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapBrush->Slot);
    if (CanvasSlot)
    {
        CurrentZoomLevel = FMath::Clamp(val, MinimumZoomLevel, MaximumZoomLevel);
        const FVector2D newSize = InitialCanvasSize * FVector2D(CurrentZoomLevel, CurrentZoomLevel);
        CanvasSlot->SetSize(newSize);
        Internal_SetCanvasPosition(CanvasSlot->GetPosition());
        UpdateMarkers();
    }
}

void UAMSMapWidget::Internal_UpdateMarkers()
{
    bPendingMarkersUpdate = false;

    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    UAMSMapSubsystem* mapSubsystem = GameInstance->GetSubsystem<UAMSMapSubsystem>();

    if (mapSubsystem)
    {
        const TArray<class UAMSMapMarkerComponent*> markers = mapSubsystem->GetAllMarkers();
        for (const auto& marker : markers)
        {
            if (markerWidgets.Contains(marker))
            {
                FAMSMarker* markerStruct = markerWidgets.FindByKey(marker);
                UpdateMarker(*markerStruct);
            } else
            {
                AddMarker(marker);
            }
        }
    }
}

void UAMSMapWidget::HandleMarkerAdded(UAMSMapMarkerComponent* marker)
{
    UpdateMarkers();
}

void UAMSMapWidget::HandleMarkerRemoved(UAMSMapMarkerComponent* marker)
{
    RemoveMarker(marker);
    UpdateMarkers();
}

void UAMSMapWidget::Internal_HandleMarkerHovered(const UAMSMarkerWidget* marker)
{
    if (markerWidgets.Contains(marker))
    {
        FAMSMarker newMarker = *markerWidgets.FindByKey(marker);
        HoveredWidget.markerComp = newMarker.markerComp;
        HoveredWidget.markerWidget = newMarker.markerWidget;
        HandleMarkerHovered(HoveredWidget);
        OnMarkerHovered.Broadcast(HoveredWidget);
    }
}

void UAMSMapWidget::Internal_HandleMarkerUnhovered(const UAMSMarkerWidget* marker)
{
    if (markerWidgets.Contains(marker))
    {
        HandleMarkerUnhovered(HoveredWidget);

        OnMarkerUnhovered.Broadcast(HoveredWidget);
        HoveredWidget.Reset();
    }
}

bool UAMSMapWidget::HasAnyTrackedMarker() const
{
    return GetMapSubsystem()->HasAnyTrackedMarker();
}

void UAMSMapWidget::TrackHoveredMarker()
{
    if (HoveredWidget.markerComp)
    {
        AAMSActorMarker* markerActor = Cast<AAMSActorMarker>(HoveredWidget.markerComp->GetOwner());
        if (markerActor)
        {
            GetMapSubsystem()->RemoveAllMarkerActors();
            OnMarkerActorsChanged.Broadcast();
            HandleMarkerActorsChanged();
        } else
        {
            if (HoveredWidget.markerComp == GetMapSubsystem()->GetCurrentlytTrackedMarker())
            {
                UntrackCurrentMarker();
            } else
            {
                UntrackCurrentMarker();
                TrackMarker(HoveredWidget);
            }
        }
    }
}

void UAMSMapWidget::TrackMarker(const FAMSMarker& marker)
{
    if (marker.ValidCheck())
    {
        GetMapSubsystem()->TrackMarker(marker.markerComp);
        marker.markerWidget->TrackMarker(true);
    }
}

void UAMSMapWidget::UntrackCurrentMarker()
{
    UAMSMapMarkerComponent* markerComp = GetMapSubsystem()->GetCurrentlytTrackedMarker();
    FAMSMarker* markerRef = markerWidgets.FindByKey(markerComp);
    if (markerRef && markerRef->ValidCheck())
    {
        markerRef->markerWidget->TrackMarker(false);
        GetMapSubsystem()->UntrackMarker();
    }
}

void UAMSMapWidget::AddMarker(UAMSMapMarkerComponent* marker)
{
    const FVector worldLoc = marker->GetOwnerLocation();
    const AAMSMapArea* mapAreaBound = GetMapArea();
    if (mapAreaBound && mapAreaBound->IsPointInThisArea(worldLoc))
    {
        UAMSMarkerWidget* widgetMarker = CreateWidget<UAMSMarkerWidget>(this, MarkersClass);
        FAMSMarker markerStruct = FAMSMarker(marker, widgetMarker);
        markerWidgets.Add(markerStruct);
        widgetMarker->SetupMarkerIcon(marker);
        widgetMarker->SetMarkerIcon(marker->GetMarkerTexture());
        widgetMarker->OnHovered.AddDynamic(this, &UAMSMapWidget::Internal_HandleMarkerHovered);
        widgetMarker->OnUnhovered.AddDynamic(this, &UAMSMapWidget::Internal_HandleMarkerUnhovered);
        /*   widgetMarker->SetMarkerSize(MarkersSize);*/
        MapCanvas->AddChildToCanvas(widgetMarker);
        UpdateMarker(markerStruct);
    }
}

void UAMSMapWidget::RemoveMarker(class UAMSMapMarkerComponent* marker)
{
    if (markerWidgets.Contains(marker))
    {
        const int32 index = markerWidgets.IndexOfByKey(marker);
        if (markerWidgets.IsValidIndex(index))
        {
            FAMSMarker markerStruct = markerWidgets[index];
            markerWidgets.Remove(markerStruct);
            if (markerStruct.markerWidget)
            {
                markerStruct.markerWidget->RemoveFromParent();
                markerStruct.markerWidget->OnHovered.RemoveDynamic(this, &UAMSMapWidget::Internal_HandleMarkerHovered);
                markerStruct.markerWidget->OnUnhovered.RemoveDynamic(this, &UAMSMapWidget::Internal_HandleMarkerUnhovered);
            }
        }
    }
}

void UAMSMapWidget::HighlightMarker(class UAMSMapMarkerComponent* marker, bool resetOtherMarkers /*= true*/)
{
    if (resetOtherMarkers)
    {
        RemoveAllMarkerHighlights();
    }
    FAMSMarker* markerWidget = markerWidgets.FindByKey(marker);
    if (markerWidget)
    {
        markerWidget->bHighlighted = true;
    }
    UpdateMarkers();
}

void UAMSMapWidget::RemoveAllMarkerHighlights()
{
    for (const auto& marker : markerWidgets)
    {
        RemoveMarkerHighlight(marker.markerComp);
    }
    UpdateMarkers();
}

void UAMSMapWidget::RemoveMarkerHighlight(class UAMSMapMarkerComponent* marker)
{
    FAMSMarker* markerWidget = markerWidgets.FindByKey(marker);
    if (markerWidget)
    {
        markerWidget->bHighlighted = false;
    }
    UpdateMarkers();
}

void UAMSMapWidget::UpdateMarkers()
{
    bPendingMarkersUpdate = true;
}

void UAMSMapWidget::UpdateMarker(FAMSMarker& marker)
{
    AAMSMapArea* mapArea = GetMapArea();

    if (mapArea && marker.markerComp && marker.markerWidget)
    {
        const FVector2D mapPos = mapArea->GetNormalized2DPositionFromWorldLocation(marker.markerComp->GetOwnerLocation());
        const FVector2D mapSize = GetMapSize();
        const FVector2D scaledPos = (mapSize * mapPos) - (MarkersSize) - FVector2D(MarkersSize.X / 2, 0.f);
        marker.markerWidget->SetRenderTranslation(scaledPos);
        if (marker.bHighlighted)
        {
            marker.markerWidget->SetRenderScale(MarkerScaleWhenHighlighted);
        } else
        {
            marker.markerWidget->SetRenderScale(FVector2D(1.f, 1.f));
        }
        if (marker.markerComp->GetShouldRotate())
        {
            const float rot = marker.markerComp->GetOwnerRotation().Yaw;
            marker.markerWidget->Rotate(rot);
        }
    }
}

void UAMSMapWidget::InitCanvas()
{
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapBrush->Slot);
    if (CanvasSlot)
    {
        InitialCanvasPosition = CanvasSlot->GetPosition();
        InitialCanvasSize = CanvasSlot->GetSize();
        InitialAlignment = CanvasSlot->GetAlignment();
    }
}

UAMSMapSubsystem* UAMSMapWidget::GetMapSubsystem() const
{
    const UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    return GameInstance->GetSubsystem<UAMSMapSubsystem>();
}

UCommonInputSubsystem* UAMSMapWidget::GetInputSubsystem() const
{
    const ULocalPlayer* BindingOwner = GetOwningLocalPlayer();
    return UCommonInputSubsystem::Get(BindingOwner);
}

void UAMSMapWidget::SetMapArea(const FName& mapArea)
{
    AreaTag = mapArea;
    const AAMSMapArea* mapAreaBound = GetMapArea();

    if (mapAreaBound)
    {
        UTexture* areaTexture = mapAreaBound->GetMapTexture();
        if (MapMaterial && MapBrush)
        {
            UMaterialInstanceDynamic* dynamicMat = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, MapMaterial);
            dynamicMat->SetTextureParameterValue(TextureParameterName, areaTexture);
            MapBrush->SetBrushFromMaterial(dynamicMat);
        }
    } else
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Area Tag! - UAMSMapWidget::SetMapArea"));
    }
}

class AAMSMapArea* UAMSMapWidget::GetMapArea() const
{
    UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(this);
    UAMSMapSubsystem* mapSubsystem = GameInstance->GetSubsystem<UAMSMapSubsystem>();
    if (mapSubsystem)
    {
        return mapSubsystem->GetRegisteredMapArea(AreaTag);
    }

    return nullptr;
}

FVector2D UAMSMapWidget::GetMapOffset() const
{
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapBrush->Slot);
    if (CanvasSlot)
    {
        return CanvasSlot->GetPosition();
    }
    UE_LOG(LogTemp, Error, TEXT("Invalid Map Widget, missing canvas slot! - UAMSMapWidget::GetMapOffset"));
    return FVector2D();
}

FVector2D UAMSMapWidget::GetMapSize() const
{
    UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(MapBrush->Slot);
    if (CanvasSlot)
    {
        return CanvasSlot->GetSize();
    }
    UE_LOG(LogTemp, Error, TEXT("Invalid Map Widget, missing canvas slot! - UAMSMapWidget::GetMapSize"));
    return FVector2D();
}