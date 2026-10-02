#pragma once

#include "CoreMinimal.h"
#include "APSStrategicMapTypes.h"
#include "Widgets/SCompoundWidget.h"

class AAstroGenerator;
class AGravityPlayerController;
class FAPSStrategicMapCamera;
class FAPSStrategicMapScene;
class SAPSStrategicMapView;
class SEditableTextBox;
class SVerticalBox;
struct FAPSObjectAction;
struct FEditableTextBoxStyle;

/**
 * F10 strategic map v2 (Rio 02.10: "the main screen for expansion, management and development, a very important part of
 * gameplay; a star list on the right: click a star to see its parameters, build routes and so on; don't hold back").
 * Its own camera over the live 3D world (FAPSStrategicMapCamera), the labelled view (SAPSStrategicMapView), and around
 * it: the header with the civilization's stocks, focus presets and layers on the left, and on the right the STARS list
 * (search, nearest to the view, known, claimed) and the OBJECT page (what the selection is, its status and every action
 * the runtime offers for it, APSObjectActions). F10 or Esc closes it through the controller's close path.
 */
class SAPSStrategicMapPanel final : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAPSStrategicMapPanel) {}
		SLATE_ARGUMENT(TWeakObjectPtr<AGravityPlayerController>, Controller)
		SLATE_ARGUMENT(TWeakObjectPtr<AAstroGenerator>, Generator)
		SLATE_EVENT(FSimpleDelegate, OnClose)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SAPSStrategicMapPanel() override;

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnPreviewKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	enum class EPreset : uint8
	{
		None,
		Cluster,
		HomeSystem,
		HomePlanet,
		MyShip,
		Selection
	};

	FReply Close();
	FReply ApplyPreset(EPreset Preset);
	/** Flies to a selection and frames it; the header names it. */
	void FocusOn(const APSStrategicMap::FSelection& Target, TOptional<double> Pitch);
	void Select(const APSStrategicMap::FSelection& Target, bool bShowObject);
	/** The first view: the last one of this world, else the system the pilot is in. */
	void OpenView();
	/** Keeps the view, layers, selection and tab for the next F10 in this world. */
	void Remember() const;
	FReply SetTab(int32 Tab);

	TSharedRef<SWidget> BuildHeader();
	TSharedRef<SWidget> BuildLeftPanel();
	TSharedRef<SWidget> BuildRightPanel();
	TSharedRef<SWidget> BuildStarsTab();
	TSharedRef<SWidget> BuildObjectTab();
	TSharedRef<SWidget> PresetButton(EPreset Preset, int32 Glyph, const FText& Label, const FText& Detail);
	TSharedRef<SWidget> LayerToggle(APSStrategicMap::ELayer Layer, const FText& Label, const FLinearColor& Swatch);
	TSharedRef<SWidget> StarRow(int32 CatalogueIndex);
	TSharedRef<SWidget> ActionButton(const TSharedRef<FAPSObjectAction>& Action);

	void UpdateStarList(float DeltaSeconds);
	/** Without bForce the rows stay when the listed systems did not change. */
	void RebuildStarList(bool bForce);
	void UpdateObjectPage(float DeltaSeconds);
	void RebuildObjectPage();
	uint32 RuntimeRevision() const;

	TWeakObjectPtr<AGravityPlayerController> Controller;
	TWeakObjectPtr<AAstroGenerator> Generator;
	FSimpleDelegate OnClose;
	TSharedPtr<FAPSStrategicMapScene> Scene;
	TSharedPtr<FAPSStrategicMapCamera> Camera;
	TSharedPtr<SAPSStrategicMapView> View;
	TSharedPtr<FEditableTextBoxStyle> SearchStyle;
	uint32 ObstaclesSerial{0};
	EPreset ActivePreset{EPreset::None};
	FText FocusTitle;
	int32 ActiveTab{0};
	bool bClosing{false};
	/** The first flight has started (it waits for the map region). */
	bool bOpened{false};
	float OpenWait{0.0f};

	// STARS
	TSharedPtr<SEditableTextBox> SearchBox;
	TSharedPtr<SVerticalBox> StarRows;
	FString SearchText;
	/** 0 nearest to the view, 1 known, 2 claimed. */
	int32 StarList{0};
	bool bStarListDirty{true};
	uint32 StarListRevision{0};
	FVector StarListFocus{FVector::ZeroVector};
	float StarListClock{0.0f};
	int32 StarListShown{0};
	TArray<int32> ListedSystems;

	// OBJECT
	TSharedPtr<SVerticalBox> ObjectBox;
	TWeakObjectPtr<AActor> ObjectTarget;
	bool bObjectHadTarget{false};
	uint32 ObjectSelectionSerial{0};
	uint32 ObjectRevision{0};
	bool bObjectDirty{true};
	float ObjectFieldsClock{0.0f};
	TArray<TPair<FText, FText>> ObjectFields;
	TArray<TSharedRef<FAPSObjectAction>> ObjectActions;
	FText ActionMessage;
	FLinearColor ActionMessageColour{FLinearColor::White};
};
