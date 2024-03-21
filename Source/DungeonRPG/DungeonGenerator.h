// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DungeonGenerator.generated.h"


class AStaticMeshActor;

USTRUCT()
struct FCoord
{
	GENERATED_BODY()

	UPROPERTY()
	int X = 0;
	UPROPERTY()
	int Y = 0;
	
	FCoord();
	FCoord(int InX, int InY);
	bool operator==(const FCoord& Other) const;
	FCoord operator+(FCoord const& obj) const;
	bool operator<(FCoord const& C2) const;
	static int GetManhattanDistanceBetweenCoords(FCoord A, FCoord B);
	static float GetEuclideanDistanceBetweenCoords(FCoord A, FCoord B);
	static TArray<FCoord> Get4AdjacentTiles(FCoord Centre);
	FCoord Inverse() const;
};

FORCEINLINE uint32 GetTypeHash(const FCoord& Coord)
{
	const uint32 Hash = FCrc::MemCrc32(&Coord, sizeof(FCoord));
	return Hash;
}


USTRUCT()
struct FCoordPair
{
	GENERATED_BODY()

	FCoord A;
	FCoord B;


	FCoordPair();
	FCoordPair(FCoord InA, FCoord InB);
	bool operator==(const FCoordPair& Other) const;

};

FORCEINLINE uint32 GetTypeHash(const FCoordPair& CoordPair)
{
	const uint32 Hash = FCrc::MemCrc32(&CoordPair, sizeof(FCoordPair));
	return Hash;
}


USTRUCT()
struct FDungeonRoom
{
	GENERATED_BODY()

	UPROPERTY()
	FCoord GlobalCentre;

	UPROPERTY()
	TArray<FCoord> LocalCoordOffsets;

	int PossibleRoomsIndex;

	FDungeonRoom();
	FDungeonRoom(FCoord InGlobalCentre, const TArray<FCoord>& InLocalCoordOffsets);
	FDungeonRoom(FCoord InGlobalCentre, const TArray<FCoord>& InLocalCoordOffsets, int InPossibleRoomsIndex);
	static int MaxManhattanDistanceBetweenRooms(TArray<FCoord> A, TArray<FCoord> B);
	static bool DoRoomsOverlap(FDungeonRoom A, FDungeonRoom B);
	static bool AreRoomsTouching(const FDungeonRoom& A, const FDungeonRoom& B);
};




UCLASS()
class DUNGEONRPG_API ADungeonGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADungeonGenerator();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Generates and spawns a layout, takes the number of rooms as an input parameter
	void GenerateLayout(int NumRooms);

	// TArray<TArray<FCoord>> PossibleRooms;
	TArray<TArray<FCoord>> InitPossibleRooms() const;

	// Matrix of Combinations of rooms. For each pair of rooms, contains the list of coordinates that room 2 can be
	// placed relative to room 1 to be adjacent.
	static TArray<FCoord> GenerateOffsetsForRooms(const TArray<FCoord>& RoomA, const TArray<FCoord>& RoomB);
	static TArray<TArray<TArray<FCoord>>> GenerateRoomComboOffsets(TArray<TArray<FCoord>> PossibleRooms);

	// Final room layout, is a list of FDungeonRooms which should all be touching each other.
	// TSet<FCoord> RoomLayoutUsedCoords = {};
	void AddSingleRoomToLayout(TArray<TArray<TArray<FCoord>>> RoomComboOffsets, TArray<TArray<FCoord>> PossibleRooms, TArray<FDungeonRoom> &RoomLayout, TSet<FCoord> &RoomLayoutUsedCoords) const;

	void SpawnMeshes(const TArray<FDungeonRoom>& RoomLayout);

	// TODO also SpawnFloor function
	void SpawnWall(FCoordPair Location, UStaticMesh* Mesh);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Actors")
	TSubclassOf<AActor> FloorActor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Actors")
	TSubclassOf<AActor> WallActor;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dungeon Actors")
	TSubclassOf<AActor> ArchwayActor;
	
	UPROPERTY(EditAnywhere)
	UStaticMesh* FloorMesh;
	UPROPERTY(EditAnywhere)
	UStaticMesh* WallMesh;
	UPROPERTY(EditAnywhere)
	UStaticMesh* ArchWayMesh;
	
	UPROPERTY(EditAnywhere)
	float FloorMeshWidth = 500;

	UPROPERTY(EditAnywhere)
	int NumOfRoomsToGenerate = 10;

	TArray<AStaticMeshActor*> FloorActors;
	TArray<AStaticMeshActor*> WallActors;

};
