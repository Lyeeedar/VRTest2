#pragma once

#include "VRHand.h"
#include "IEquippable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UEquippable : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

/**
 * 
 */
class VRCODE_API IEquippable
{
	GENERATED_IINTERFACE_BODY()

public:
	
	UFUNCTION( BlueprintNativeEvent, BlueprintCallable, Category = "Equippable Interface" )
	void Equip( class AVRHand *AttachTo );

	UFUNCTION( BlueprintNativeEvent, BlueprintCallable, Category = "Equippable Interface" )
	void Unequip();

	UFUNCTION( BlueprintNativeEvent, BlueprintCallable, Category = "Equippable Interface" )
	void Use();
	
};
