#include "TFBroadcastComponent.h"

#include "ROSIntegrationGameInstance.h"
#include "tf2_msgs/TFMessage.h"
#include "ROSTime.h"

// Sets default values for this component's properties
UTFBroadcastComponent::UTFBroadcastComponent()
: ComponentActive(true)
, FrameRate(1)
, CoordsRelativeTo(ECoordinateType::COORDTYPE_WORLD)
, ParentFrameName(TEXT("/world"))
, ThisFrameName(TEXT("/tfbroadcast_default"))
, UseParentActorLabelAsParentFrame(false)
, UseActorLabelAsFrame(false)
, FrameTime(1.0f / FrameRate)
, TimePassed(0)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these
	// features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

// Called when the game starts
DEFINE_LOG_CATEGORY_STATIC(LogTFBroadcastComponent, Log, All);
DEFINE_LOG_CATEGORY_STATIC(LogTFBroadcastComponentTick, Log, All);

void UTFBroadcastComponent::BeginPlay()
{
    Super::BeginPlay();

    // Check if the owner exists
    if (!GetOwner())
    {
        UE_LOG(LogTFBroadcastComponent, Error, TEXT("UTFBroadcastComponent::BeginPlay() - GetOwner() returned nullptr."));
        return;
    }

    _TFTopic = NewObject<UTopic>(UTopic::StaticClass());
    if (!_TFTopic)
    {
        UE_LOG(LogTFBroadcastComponent, Error, TEXT("UTFBroadcastComponent::BeginPlay() - Failed to create UTopic object."));
        return;
    }

    UROSIntegrationGameInstance* ROSInstance = Cast<UROSIntegrationGameInstance>(GetOwner()->GetGameInstance());
    if (!ROSInstance)
    {
        UE_LOG(LogTFBroadcastComponent, Error, TEXT("UTFBroadcastComponent::BeginPlay() - Failed to cast GameInstance to UROSIntegrationGameInstance."));
        return;
    }

    _TFTopic->Init(ROSInstance->ROSIntegrationCore, TEXT("/tf"), TEXT("tf2_msgs/TFMessage"));
    UE_LOG(LogTFBroadcastComponent, Log, TEXT("UTFBroadcastComponent::BeginPlay() - TF Topic initialized."));
}

AActor* UTFBroadcastComponent::GetParentActor()
{
	auto RootComponent = GetOwner()->GetRootComponent();
	assert(RootComponent);
	if (!(RootComponent->GetAttachParent()))
		return nullptr;

	return RootComponent->GetAttachParent()->GetOwner();
}


void UTFBroadcastComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    // Log the beginning of a Tick
    UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - Start"));

    // Check for framerate
    TimePassed += DeltaTime;
    if (TimePassed < FrameTime) {
        UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - Skipping this frame based on FrameTime"));
        return;
    }
    TimePassed -= FrameTime;

    TickCounter++;

    bool GlobalSettingTFBroadcastEnabled = false;

    /*auto World = GetWorld();
    if (World) {
        auto WorldSettings = World->GetWorldSettings();
        if (WorldSettings) {
            AMyWorldSettings* MyWorldSettings = Cast<AMyWorldSettings>(WorldSettings);
            GlobalSettingTFBroadcastEnabled = MyWorldSettings->bEnableTFBroadcast;
            // Log MyWorldSettings::bEnableTFBroadcast value
            UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("MyWorldSettings::bEnableTFBroadcast is %s"), GlobalSettingTFBroadcastEnabled ? TEXT("True") : TEXT("False"));
        }
        else {
            UE_LOG(LogTFBroadcastComponentTick, Warning, TEXT("Failed to GetWorldSettings() - Can't access WorldSettings"));
        }
    }
    else {
        UE_LOG(LogTFBroadcastComponentTick, Warning, TEXT("Failed to GetWorld() - Can't access World"));
    }*/

    // Log owner location
    if (GetOwner()) {
        UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("Owner Loc: %s"), *(GetOwner()->GetActorLocation().ToString()));
    } else {
        UE_LOG(LogTFBroadcastComponentTick, Error, TEXT("TickComponent - GetOwner() returned nullptr."));
        return;
    }

    TickCounter = 0;

    // Skip execution when TF is deactivated globally
    //if (!GlobalSettingTFBroadcastEnabled) return;

    if (!ComponentActive) {
        UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - Component is not active, skipping"));
        return;
    }

    // Setup the Frame Names
    FString CurrentThisFrameName = ThisFrameName;
#if WITH_EDITOR
    if (UseActorLabelAsFrame) {
        CurrentThisFrameName = GetOwner()->GetActorLabel();
    }
#endif // WITH_EDITOR

    FString CurrentParentFrameName = ParentFrameName;
#if WITH_EDITOR
    if (UseParentActorLabelAsParentFrame) {
        AActor* ParentActor = GetParentActor();
        if (ParentActor) {
            CurrentParentFrameName = ParentActor->GetActorLabel();
            CoordsRelativeTo = ECoordinateType::COORDTYPE_RELATIVE;
        } else {
            UE_LOG(LogTFBroadcastComponentTick, Error, TEXT("[TFBroadcast] UseParentActorLabelAsParentFrame==true and No Parent Component on %s - Add a parent actor or deactivate UseParentActorLabelAsParentFrame"), *(GetOwner()->GetActorLabel()));
            return;
        }
    }
#endif // WITH_EDITOR

    FVector ActorTranslation;
    FQuat ActorRotation;

    if (CoordsRelativeTo == ECoordinateType::COORDTYPE_RELATIVE) {
        AActor* ParentActor = GetParentActor();
        if (!ParentActor) {
#if WITH_EDITOR
            UE_LOG(LogTFBroadcastComponentTick, Error, TEXT("[TFBroadcast] CoordsRelativeTo == ECoordinateType::COORDTYPE_RELATIVE and No Parent Component on %s - Add a parent actor or use world coordinates. Skipping TF Broadcast"), *(GetOwner()->GetActorLabel()));
#else
            UE_LOG(LogTFBroadcastComponentTick, Error, TEXT("[TFBroadcast] CoordsRelativeTo == ECoordinateType::COORDTYPE_RELATIVE and No Parent Component - Add a parent actor or use world coordinates. Skipping TF Broadcast"));
#endif // WITH_EDITOR
            return;
        }
        FTransform ThisTransformInWorldCoordinates = GetOwner()->GetRootComponent()->GetComponentTransform();
        FTransform ParentTransformInWorldCoordinates = ParentActor->GetRootComponent()->GetComponentTransform();
        FTransform RelativeTransform = ThisTransformInWorldCoordinates.GetRelativeTransform(ParentTransformInWorldCoordinates);
        ActorTranslation = RelativeTransform.GetLocation();
        ActorRotation = RelativeTransform.GetRotation();
    } else {
        ActorTranslation = GetOwner()->GetActorLocation();
        ActorRotation = GetOwner()->GetActorQuat();
    }

    // Log the calculated transformation
	UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - Actor Translation: %s, Actor Rotation: %s"), *ActorTranslation.ToString(), *ActorRotation.ToString());

	double TranslationX = -ActorTranslation.X / 100.0f;
	UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - TranslationX calculated: %f"), TranslationX);

	double TranslationY = ActorTranslation.Y / 100.0f;
	UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - TranslationY calculated: %f"), TranslationY);

	double TranslationZ = -ActorTranslation.Z / 100.0f;
	UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - TranslationZ calculated: %f"), TranslationZ);

	// double RotationX = -ActorRotation.X;
	 double RotationX = 0;
	UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - RotationX calculated: %f"), RotationX);

	// double RotationY = ActorRotation.Y;
	 double RotationY = 0;
	UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - RotationY calculated: %f"), RotationY);

	// double RotationZ = -ActorRotation.Z;
	double RotationZ = 0;
	UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - RotationZ calculated: %f"), RotationZ);

	// double RotationW = ActorRotation.W;
	 double RotationW = 1;
	UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - RotationW calculated: %f"), RotationW);

	FROSTime time = FROSTime::Now();
	UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - FROSTime::Now() called"));

	TSharedPtr<ROSMessages::tf2_msgs::TFMessage> TFMessage(new ROSMessages::tf2_msgs::TFMessage());
	if (!TFMessage.IsValid()) {
		UE_LOG(LogTFBroadcastComponentTick, Error, TEXT("TickComponent - Failed to create new ROSMessages::tf2_msgs::TFMessage"));
		return;
}

ROSMessages::geometry_msgs::TransformStamped TransformStamped;
TransformStamped.header.seq = 0;
TransformStamped.header.time = time;
TransformStamped.header.frame_id = CurrentParentFrameName;
TransformStamped.child_frame_id = CurrentThisFrameName;
TransformStamped.transform.translation.x = TranslationX;
TransformStamped.transform.translation.y = TranslationY;
TransformStamped.transform.translation.z = TranslationZ;
TransformStamped.transform.rotation.x = RotationX;
TransformStamped.transform.rotation.y = RotationY;
TransformStamped.transform.rotation.z = RotationZ;
TransformStamped.transform.rotation.w = RotationW;

UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - TransformStamped prepared"));

TFMessage->transforms.Add(TransformStamped);
UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - TransformStamped added to TFMessage"));

_TFTopic->Publish(TFMessage);
UE_LOG(LogTFBroadcastComponentTick, Log, TEXT("TickComponent - TF Message published."));

}

void UTFBroadcastComponent::SetFramerate(const float _FrameRate)
{
	FrameRate = _FrameRate;
	FrameTime = 1.0f / _FrameRate;
	TimePassed = 0;
}
