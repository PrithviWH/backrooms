// Copyright Epic Games, Inc. All Rights Reserved.

#include "BackroomsAssetManager.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Engine.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundMix.h"
#include "Sound/SoundClass.h"
#include "InputMappingContext.h"
#include "InputAction.h"

UBackroomsAssetManager::UBackroomsAssetManager()
{
	// Configure default soft object paths to preserve asset bindings
	CarpetPhysicalMaterial = TSoftObjectPtr<UPhysicalMaterial>(FSoftObjectPath(TEXT("/Game/Backrooms/Materials/PM_Carpet.PM_Carpet")));

	CarpetFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step1.Step1"))));
	CarpetFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step2.Step2"))));
	CarpetFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step3.Step3"))));
	CarpetFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step4.Step4"))));
	CarpetFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/Footsteps/Step5.Step5"))));

	HeavyFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/lordsonny-monster-footstep-162883.lordsonny-monster-footstep-162883"))));
	HeavyFootstepSounds.Add(TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/universfield-heavy-walking-footsteps-352771.universfield-heavy-walking-footsteps-352771"))));

	ConcreteFootstepSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Foot-steps/freeeverythingxx-walking-on-concrete-ver-2-268513.freeeverythingxx-walking-on-concrete-ver-2-268513")));
	SicklyBreathingSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ghost_sfx/freesound_community-sickly-breathing-83152.freesound_community-sickly-breathing-83152")));
	ZombieGrowlSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ghost_sfx/mixkit-zombie-monster-growl-1973.mixkit-zombie-monster-growl-1973")));
	HeartbeatSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Heartbeat/lenspulse-intense-cinematic-heartbeat-sound-effect-584627.lenspulse-intense-cinematic-heartbeat-sound-effect-584627")));
	OutOfBreathSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Heartbeat/voicebosch-out-of-breath-male-176715.voicebosch-out-of-breath-male-176715")));
	EntitySightStingerSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ghost_sfx/universfield-scary-string-tension-454849.universfield-scary-string-tension-454849")));

	AtmosphereSuspenseDrone = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ambience/universfield-horror-background-atmosphere-for-suspense-166944.universfield-horror-background-atmosphere-for-suspense-166944")));
	AtmosphereTrailerDrone = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ambience/simplesound-horror-trailer-443327.simplesound-horror-trailer-443327")));
	AtmosphereRoomTone = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ambience/soundsforyou-the-ambience-room-tone-139064.soundsforyou-the-ambience-room-tone-139064")));

	CreepyDoorSlamSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Distant/freesound_community-door-slam-2-47114.freesound_community-door-slam-2-47114")));
	CreepyDoorKnockSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Door-Knock/freesound_community-door-knock-47175.freesound_community-door-knock-47175")));
	CreepyDistantThudSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Distant/freesound_community-quiet-thud-hits-100179.freesound_community-quiet-thud-hits-100179")));
	CreepyHeavyFallingSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Distant/universfield-heavy-object-falling-291096.universfield-heavy-object-falling-291096")));
	CreepyReverbImpactSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Distant/black_kumizhi-huge-cinematic-reverb-impact-506132.black_kumizhi-huge-cinematic-reverb-impact-506132")));
	CreepyWhisperSenseSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Distant/phatphrogstudio-lich-demonic-voice-i-sense-you-490452.phatphrogstudio-lich-demonic-voice-i-sense-you-490452")));
	CreepyPossessedLaughSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ghost_sfx/alesiadavina-creepy-horror-sound-possessed-2-laughter-vol-001-167400.alesiadavina-creepy-horror-sound-possessed-2-laughter-vol-001-167400")));
	CreepyClockTickSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ambience/soundreality-cinematic-horror-clock-425037.soundreality-cinematic-horror-clock-425037")));

	VoiceWhisperOverHere = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Voices/voice_whisper_over_here.voice_whisper_over_here")));
	VoiceWhisperDontLookBack = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Voices/voice_whisper_dont_look_back.voice_whisper_dont_look_back")));
	VoiceWhisperNoEscape = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Voices/voice_whisper_no_escape.voice_whisper_no_escape")));
	VoiceWhisperCounting = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Voices/voice_whisper_counting.voice_whisper_counting")));
	VoiceWhisperWhyRunning = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Voices/voice_whisper_why_running.voice_whisper_why_running")));

	SFXSubtlePipeDrip = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SubtleSFX/sfx_subtle_pipe_drip.sfx_subtle_pipe_drip")));
	SFXSubtleWallGroan = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SubtleSFX/sfx_subtle_wall_groan.sfx_subtle_wall_groan")));
	SFXSubtleVentDraft = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SubtleSFX/sfx_subtle_vent_draft.sfx_subtle_vent_draft")));
	SFXSubtleLightArc = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SubtleSFX/sfx_subtle_light_flicker_arc.sfx_subtle_light_flicker_arc")));
	SFXSubtleCeilingScuttle = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SubtleSFX/sfx_subtle_ceiling_scuttle.sfx_subtle_ceiling_scuttle")));

	JumpscareScreamSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ghost_sfx/jusatti890-scream-horror-sfx-490908.jusatti890-scream-horror-sfx-490908")));
	JumpscareHitSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ghost_sfx/mixkit-hard-horror-hit-drum-565.mixkit-hard-horror-hit-drum-565")));
	JumpscareImpactSound = TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/Ghost_sfx/universfield-horror-impact-hit-567238.universfield-horror-impact-hit-567238")));
	MonsterAttackAnimation = TSoftObjectPtr<UAnimSequence>(FSoftObjectPath(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01")));

	DefaultMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_Default.IMC_Default")));
	MouseLookMappingContext = TSoftObjectPtr<UInputMappingContext>(FSoftObjectPath(TEXT("/Game/Input/IMC_MouseLook.IMC_MouseLook")));

	MoveAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Move.IA_Move")));
	LookAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Look.IA_Look")));
	MouseLookAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_MouseLook.IA_MouseLook")));
	JumpAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Jump.IA_Jump")));
	SprintAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Sprint.IA_Sprint")));
	PauseAction = TSoftObjectPtr<UInputAction>(FSoftObjectPath(TEXT("/Game/Input/Actions/IA_Pause.IA_Pause")));

	BackroomsSoundMix = TSoftObjectPtr<USoundMix>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SM_BackroomsVolume.SM_BackroomsVolume")));
	MasterSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SoundClasses/SC_Master.SC_Master")));
	SFXSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SoundClasses/SC_SFX.SC_SFX")));
	AmbienceSoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SoundClasses/SC_Ambience.SC_Ambience")));
	EntitySoundClass = TSoftObjectPtr<USoundClass>(FSoftObjectPath(TEXT("/Game/Backrooms/Audio/SoundClasses/SC_Entity.SC_Entity")));
}

UBackroomsAssetManager& UBackroomsAssetManager::Get()
{
	if (UBackroomsAssetManager* Singleton = Cast<UBackroomsAssetManager>(GEngine ? GEngine->AssetManager.Get() : nullptr))
	{
		return *Singleton;
	}

	return *GetMutableDefault<UBackroomsAssetManager>();
}

void UBackroomsAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();

	UE_LOG(LogTemp, Log, TEXT("[BackroomsAssetManager] Initializing and validating asset references..."));
	TArray<FString> MissingAssets;
	if (!ValidateAssetReferences(MissingAssets))
	{
		for (const FString& Missing : MissingAssets)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BackroomsAssetManager] Missing or unverified asset: %s"), *Missing);
		}
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[BackroomsAssetManager] All primary soft assets validated successfully in AssetRegistry."));
	}
}

bool UBackroomsAssetManager::ValidateAssetReferences(TArray<FString>& OutMissingAssets)
{
	OutMissingAssets.Empty();

	IAssetRegistry* AssetRegistry = IAssetRegistry::Get();
	if (!AssetRegistry)
	{
		return true; // Asset registry not yet available in early phase
	}

	auto CheckSoftPath = [&](const FSoftObjectPath& SoftPath, const FString& Label)
	{
		if (SoftPath.IsNull())
		{
			return;
		}
		FAssetData AssetData = AssetRegistry->GetAssetByObjectPath(SoftPath);
		if (!AssetData.IsValid())
		{
			OutMissingAssets.Add(FString::Printf(TEXT("%s: %s"), *Label, *SoftPath.ToString()));
		}
	};

	CheckSoftPath(CarpetPhysicalMaterial.ToSoftObjectPath(), TEXT("CarpetPhysicalMaterial"));

	for (int32 i = 0; i < CarpetFootstepSounds.Num(); ++i)
	{
		CheckSoftPath(CarpetFootstepSounds[i].ToSoftObjectPath(), FString::Printf(TEXT("CarpetFootstepSounds[%d]"), i));
	}
	for (int32 i = 0; i < HeavyFootstepSounds.Num(); ++i)
	{
		CheckSoftPath(HeavyFootstepSounds[i].ToSoftObjectPath(), FString::Printf(TEXT("HeavyFootstepSounds[%d]"), i));
	}
	CheckSoftPath(ConcreteFootstepSound.ToSoftObjectPath(), TEXT("ConcreteFootstepSound"));
	CheckSoftPath(SicklyBreathingSound.ToSoftObjectPath(), TEXT("SicklyBreathingSound"));
	CheckSoftPath(ZombieGrowlSound.ToSoftObjectPath(), TEXT("ZombieGrowlSound"));
	CheckSoftPath(EntitySightStingerSound.ToSoftObjectPath(), TEXT("EntitySightStingerSound"));
	CheckSoftPath(AtmosphereSuspenseDrone.ToSoftObjectPath(), TEXT("AtmosphereSuspenseDrone"));
	CheckSoftPath(AtmosphereTrailerDrone.ToSoftObjectPath(), TEXT("AtmosphereTrailerDrone"));
	CheckSoftPath(AtmosphereRoomTone.ToSoftObjectPath(), TEXT("AtmosphereRoomTone"));
	CheckSoftPath(CreepyDoorSlamSound.ToSoftObjectPath(), TEXT("CreepyDoorSlamSound"));
	CheckSoftPath(CreepyDoorKnockSound.ToSoftObjectPath(), TEXT("CreepyDoorKnockSound"));
	CheckSoftPath(CreepyDistantThudSound.ToSoftObjectPath(), TEXT("CreepyDistantThudSound"));
	CheckSoftPath(CreepyHeavyFallingSound.ToSoftObjectPath(), TEXT("CreepyHeavyFallingSound"));
	CheckSoftPath(CreepyReverbImpactSound.ToSoftObjectPath(), TEXT("CreepyReverbImpactSound"));
	CheckSoftPath(CreepyWhisperSenseSound.ToSoftObjectPath(), TEXT("CreepyWhisperSenseSound"));
	CheckSoftPath(CreepyPossessedLaughSound.ToSoftObjectPath(), TEXT("CreepyPossessedLaughSound"));
	CheckSoftPath(CreepyClockTickSound.ToSoftObjectPath(), TEXT("CreepyClockTickSound"));
	CheckSoftPath(VoiceWhisperOverHere.ToSoftObjectPath(), TEXT("VoiceWhisperOverHere"));
	CheckSoftPath(VoiceWhisperDontLookBack.ToSoftObjectPath(), TEXT("VoiceWhisperDontLookBack"));
	CheckSoftPath(VoiceWhisperNoEscape.ToSoftObjectPath(), TEXT("VoiceWhisperNoEscape"));
	CheckSoftPath(VoiceWhisperCounting.ToSoftObjectPath(), TEXT("VoiceWhisperCounting"));
	CheckSoftPath(VoiceWhisperWhyRunning.ToSoftObjectPath(), TEXT("VoiceWhisperWhyRunning"));
	CheckSoftPath(SFXSubtlePipeDrip.ToSoftObjectPath(), TEXT("SFXSubtlePipeDrip"));
	CheckSoftPath(SFXSubtleWallGroan.ToSoftObjectPath(), TEXT("SFXSubtleWallGroan"));
	CheckSoftPath(SFXSubtleVentDraft.ToSoftObjectPath(), TEXT("SFXSubtleVentDraft"));
	CheckSoftPath(SFXSubtleLightArc.ToSoftObjectPath(), TEXT("SFXSubtleLightArc"));
	CheckSoftPath(SFXSubtleCeilingScuttle.ToSoftObjectPath(), TEXT("SFXSubtleCeilingScuttle"));
	CheckSoftPath(JumpscareScreamSound.ToSoftObjectPath(), TEXT("JumpscareScreamSound"));
	CheckSoftPath(JumpscareHitSound.ToSoftObjectPath(), TEXT("JumpscareHitSound"));
	CheckSoftPath(JumpscareImpactSound.ToSoftObjectPath(), TEXT("JumpscareImpactSound"));
	CheckSoftPath(MonsterAttackAnimation.ToSoftObjectPath(), TEXT("MonsterAttackAnimation"));

	CheckSoftPath(DefaultMappingContext.ToSoftObjectPath(), TEXT("DefaultMappingContext"));
	CheckSoftPath(MouseLookMappingContext.ToSoftObjectPath(), TEXT("MouseLookMappingContext"));
	CheckSoftPath(MoveAction.ToSoftObjectPath(), TEXT("MoveAction"));
	CheckSoftPath(LookAction.ToSoftObjectPath(), TEXT("LookAction"));
	CheckSoftPath(MouseLookAction.ToSoftObjectPath(), TEXT("MouseLookAction"));
	CheckSoftPath(JumpAction.ToSoftObjectPath(), TEXT("JumpAction"));
	CheckSoftPath(SprintAction.ToSoftObjectPath(), TEXT("SprintAction"));
	CheckSoftPath(PauseAction.ToSoftObjectPath(), TEXT("PauseAction"));

	CheckSoftPath(BackroomsSoundMix.ToSoftObjectPath(), TEXT("BackroomsSoundMix"));
	CheckSoftPath(MasterSoundClass.ToSoftObjectPath(), TEXT("MasterSoundClass"));
	CheckSoftPath(SFXSoundClass.ToSoftObjectPath(), TEXT("SFXSoundClass"));
	CheckSoftPath(AmbienceSoundClass.ToSoftObjectPath(), TEXT("AmbienceSoundClass"));
	CheckSoftPath(EntitySoundClass.ToSoftObjectPath(), TEXT("EntitySoundClass"));

	return OutMissingAssets.Num() == 0;
}

TSharedPtr<FStreamableHandle> UBackroomsAssetManager::PreloadCriticalAssetsAsync(FStreamableDelegate OnComplete)
{
	TArray<FSoftObjectPath> Targets;

	auto AddTarget = [&](const FSoftObjectPath& SoftPath)
	{
		if (!SoftPath.IsNull())
		{
			Targets.Add(SoftPath);
		}
	};

	AddTarget(CarpetPhysicalMaterial.ToSoftObjectPath());
	for (const auto& Snd : CarpetFootstepSounds) AddTarget(Snd.ToSoftObjectPath());
	for (const auto& Snd : HeavyFootstepSounds) AddTarget(Snd.ToSoftObjectPath());
	AddTarget(ConcreteFootstepSound.ToSoftObjectPath());
	AddTarget(SicklyBreathingSound.ToSoftObjectPath());
	AddTarget(ZombieGrowlSound.ToSoftObjectPath());
	AddTarget(EntitySightStingerSound.ToSoftObjectPath());
	AddTarget(AtmosphereSuspenseDrone.ToSoftObjectPath());
	AddTarget(AtmosphereTrailerDrone.ToSoftObjectPath());
	AddTarget(AtmosphereRoomTone.ToSoftObjectPath());
	AddTarget(CreepyDoorSlamSound.ToSoftObjectPath());
	AddTarget(CreepyDoorKnockSound.ToSoftObjectPath());
	AddTarget(CreepyDistantThudSound.ToSoftObjectPath());
	AddTarget(CreepyHeavyFallingSound.ToSoftObjectPath());
	AddTarget(CreepyReverbImpactSound.ToSoftObjectPath());
	AddTarget(CreepyWhisperSenseSound.ToSoftObjectPath());
	AddTarget(CreepyPossessedLaughSound.ToSoftObjectPath());
	AddTarget(CreepyClockTickSound.ToSoftObjectPath());
	AddTarget(VoiceWhisperOverHere.ToSoftObjectPath());
	AddTarget(VoiceWhisperDontLookBack.ToSoftObjectPath());
	AddTarget(VoiceWhisperNoEscape.ToSoftObjectPath());
	AddTarget(VoiceWhisperCounting.ToSoftObjectPath());
	AddTarget(VoiceWhisperWhyRunning.ToSoftObjectPath());
	AddTarget(SFXSubtlePipeDrip.ToSoftObjectPath());
	AddTarget(SFXSubtleWallGroan.ToSoftObjectPath());
	AddTarget(SFXSubtleVentDraft.ToSoftObjectPath());
	AddTarget(SFXSubtleLightArc.ToSoftObjectPath());
	AddTarget(SFXSubtleCeilingScuttle.ToSoftObjectPath());
	AddTarget(JumpscareScreamSound.ToSoftObjectPath());
	AddTarget(JumpscareHitSound.ToSoftObjectPath());
	AddTarget(JumpscareImpactSound.ToSoftObjectPath());
	AddTarget(MonsterAttackAnimation.ToSoftObjectPath());

	AddTarget(DefaultMappingContext.ToSoftObjectPath());
	AddTarget(MouseLookMappingContext.ToSoftObjectPath());
	AddTarget(MoveAction.ToSoftObjectPath());
	AddTarget(LookAction.ToSoftObjectPath());
	AddTarget(MouseLookAction.ToSoftObjectPath());
	AddTarget(JumpAction.ToSoftObjectPath());
	AddTarget(SprintAction.ToSoftObjectPath());
	AddTarget(PauseAction.ToSoftObjectPath());

	AddTarget(BackroomsSoundMix.ToSoftObjectPath());
	AddTarget(MasterSoundClass.ToSoftObjectPath());
	AddTarget(SFXSoundClass.ToSoftObjectPath());
	AddTarget(AmbienceSoundClass.ToSoftObjectPath());
	AddTarget(EntitySoundClass.ToSoftObjectPath());

	if (Targets.Num() == 0)
	{
		OnComplete.ExecuteIfBound();
		return nullptr;
	}

	return GetStreamableManager().RequestAsyncLoad(Targets, OnComplete);
}
