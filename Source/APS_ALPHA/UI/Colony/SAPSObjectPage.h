#pragma once

#include "CoreMinimal.h"
#include "APS_ALPHA/Gameplay/Expansion/APSInfrastructureCatalog.h"
#include "APS_ALPHA/UI/Style/APSMenuChrome.h"
#include "Widgets/SCompoundWidget.h"

class AActor;
class APlanetaryBody;
class SAPSSurfaceMap;
class SBox;
class STextBlock;
class SVerticalBox;
class UWorld;
struct FAPSObjectAction;

/** Opens an object's page: the network map, the lists and the page's own rows ask for it. */
DECLARE_DELEGATE_OneParam(FAPSOnOpenObject, AActor*);

/** Pieces shared by the INFRASTRUCTURE screen's widgets (SAPSInfrastructurePanel, SAPSObjectPage), in the terminal's look. */
namespace APSInfrastructureUI
{
	/** The terminal's chamfered button: a card whose frame lights up on hover or when selected. */
	TSharedRef<SWidget> FrameButton(TSharedRef<SWidget> Content, FOnClicked OnClicked, TAttribute<bool> IsSelected,
		const FLinearColor& Accent);
	/** A label alone (chips, CLOSE, BACK): centred both ways in the button by its capitals (Rio 03.10: "everywhere the
	 * text strictly centred by height and width"); a label with its own render transform (a symbol) keeps it. */
	TSharedRef<SWidget> FrameButton(const TSharedRef<STextBlock>& Label, FOnClicked OnClicked, TAttribute<bool> IsSelected,
		const FLinearColor& Accent);
	/** One action: filled amber with an edge in its colour when it can be done, dim when it cannot (the terminal's
	 * primary button). */
	TSharedRef<SWidget> FilledButton(const FText& Label, FOnClicked OnClicked, TAttribute<bool> CanClick,
		const FLinearColor& Accent);
	/**
	 * Rio 04.10: an object's action as a FilledButton with the reason under it; while its order is under way
	 * (FAPSObjectAction::Underway) the button turns dark, fills from the left in the action's colour as the ship flies
	 * there and works, shows the percentage, and says under it who is doing what. Width: the cell's width.
	 */
	TSharedRef<SWidget> ActionCell(const FAPSObjectAction& Action, FOnClicked OnClicked, bool bCanRun, float Width);
	/** A small rounded chip with a colour swatch: resources, categories, effects. */
	TSharedRef<SWidget> Chip(const TAttribute<FText>& Text, const TAttribute<FSlateColor>& TextColour,
		const FLinearColor& Swatch);
	/** A catalogue category's glyph: outposts, stations, relays, transport, megastructures. */
	EAPSChromeGlyph CategoryGlyph(APSInfrastructure::ECategory Category);
	/** The line glyph and colour an object is drawn with on the screen's lists, map and page. */
	EAPSChromeGlyph GlyphOf(const AActor* Object);
	FLinearColor ColourOf(const AActor* Object);
	/** The home star: it stands for the home system in construction and on the maps. */
	AActor* HomeStar(const UWorld* World);
	/** The planet or moon something orbits or stands on: its attach parents, else the world whose surface it is on. */
	APlanetaryBody* BodyOf(const AActor* Actor);
}

/**
 * The object page of the colony terminal's INFRASTRUCTURE screen (Rio 02.10: "I don't like this little popup: OPEN should
 * open a full page; more control: status and information for the selected object, a planet or a technical object, and
 * the ways to interact with it; every kind of object may have its own options, it must be extensible"). One page for
 * any object: its kind and name with a preview at the side (a planet's or moon's own globe, else its glyph), the
 * runtime's status lines (APSObjectActions::Describe), what stands there and what it belongs to, the fleet's units under
 * way there, and every action the providers offer for it (APSObjectActions::Gather) plus the host screen's own, as
 * buttons by group: a dim button says why under it, a click runs the action and shows what it answered.
 *
 * Read live while shown; each part is rebuilt only when what it lists changes.
 */
class SAPSObjectPage final : public SCompoundWidget
{
public:
	/** The host screen's own actions for an object (switching to its tabs), added after the providers' ones. */
	using FExtraActions = TFunction<void(AActor*, TArray<FAPSObjectAction>&)>;

	SLATE_BEGIN_ARGS(SAPSObjectPage) {}
		SLATE_ARGUMENT(TWeakObjectPtr<UWorld>, World)
		SLATE_ARGUMENT(FExtraActions, ExtraActions)
		/** BACK, and what it returns to. */
		SLATE_EVENT(FSimpleDelegate, OnBack)
		SLATE_ATTRIBUTE(FText, BackLabel)
		/** A row of the page names another object: its page. */
		SLATE_EVENT(FAPSOnOpenObject, OnOpenObject)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual void Tick(const FGeometry& AllottedGeometry, double InCurrentTime, float InDeltaTime) override;

	/** Shows an object: everything is read again at once. */
	void SetPageObject(AActor* Object);
	AActor* GetPageObject() const { return PageActor.Get(); }

private:
	/** A row naming another object: what stands here, what this belongs to. */
	struct FRelated
	{
		TWeakObjectPtr<AActor> Actor;
		FText Name;
		FText Detail;
		EAPSChromeGlyph Glyph{EAPSChromeGlyph::Infrastructure};
		FLinearColor Colour{FLinearColor::White};
	};

	/** Reads the parts again: the status lines and units often, the rows and actions every second (bForce: all now). */
	void Refresh(bool bForce);
	void RefreshFields();
	void RefreshRelated();
	void RefreshUnderWay();
	void RefreshActions();
	void CollectHere(const AActor* Object, TArray<FRelated>& OutRows) const;
	void CollectLinked(const AActor* Object, TArray<FRelated>& OutRows) const;
	void FillRows(const TSharedPtr<SVerticalBox>& Box, const TArray<FRelated>& Rows, const FText& Empty);
	void GatherActions(TArray<FAPSObjectAction>& OutActions) const;
	FReply RunAction(FName ActionId);
	FReply OpenRelated(TWeakObjectPtr<AActor> Actor);
	/** The preview: the globe for a planet or moon with a surface, else the object's glyph in its colour. */
	void UpdatePreview();
	bool IsGlobeShown() const;

	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AActor> PageActor;
	FExtraActions ExtraActions;
	FSimpleDelegate OnBack;
	FAPSOnOpenObject OnOpenObject;
	TAttribute<FText> BackLabel;

	TSharedPtr<SAPSSurfaceMap> Globe;
	TSharedPtr<SBox> GlyphBox;
	TSharedPtr<SVerticalBox> FieldsBox;
	TSharedPtr<SVerticalBox> HereBox;
	TSharedPtr<SVerticalBox> LinkedBox;
	TSharedPtr<SVerticalBox> UnderWayBox;
	TSharedPtr<SVerticalBox> ActionsBox;

	/** The status lines as last read; the rows show them by index, rebuilt when the labels change. */
	TArray<TPair<FText, FText>> Fields;
	FString FieldsSignature;
	FString RelatedSignature;
	FString UnderWaySignature;
	FString ActionsSignature;
	FText Message;
	FLinearColor MessageColour{FLinearColor::White};
	float FastClock{0.0f};
	float SlowClock{0.0f};
	double LastTickSeconds{0.0};
};
