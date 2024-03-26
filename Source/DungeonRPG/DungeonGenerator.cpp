// Fill out your copyright notice in the Description page of Project Settings.


#include "DungeonGenerator.h"

#include "Kismet/KismetMathLibrary.h"


// Sets default values
ADungeonGenerator::ADungeonGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ADungeonGenerator::BeginPlay()
{
	Super::BeginPlay();
	
	const FDateTime StartTime = FDateTime::UtcNow();
	GenerateLayout(NumOfRoomsToGenerate);
	const float TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
	UE_LOG(LogTemp, Display, TEXT("Total Startup in %fms"), TimeElapsedInMs)
	
}

// Called every frame
void ADungeonGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}



FCoord::FCoord()
{
	X = 0;
	Y = 0;
}
	
FCoord::FCoord(int InX, int InY)
{
	X = InX;
	Y = InY;
}

bool FCoord::operator== (const FCoord& Other) const
{
	return (X == Other.X &&
			Y == Other.Y);
}

FCoord FCoord::operator+(FCoord const& obj) const
{
	FCoord res;
	res.X = X + obj.X;
	res.Y = Y + obj.Y;
	return res;
}

bool FCoord::operator< (FCoord const& C2) const
{
	return GetEuclideanDistanceBetweenCoords(*this, FCoord()) < GetEuclideanDistanceBetweenCoords(C2, FCoord());
}

int FCoord::GetManhattanDistanceBetweenCoords(FCoord A, FCoord B)
{
	return abs(A.X - B.X) + abs(A.Y - B.Y);
}

float FCoord::GetEuclideanDistanceBetweenCoords(FCoord A, FCoord B)
{
	return UKismetMathLibrary::Sqrt(((A.X - B.X)*(A.X - B.X)) + ((A.Y - B.Y)*(A.Y - B.Y)));
}

TArray<FCoord> FCoord::Get4AdjacentTiles(FCoord Centre)
{
	TArray<FCoord> AdjacentCoords = {
		Centre+FCoord(0,-1),
		Centre+FCoord(1,0),
		Centre+FCoord(0,1),
		Centre+FCoord(-1,0)};
	return AdjacentCoords;
}

FCoord FCoord::Inverse() const
{
	return FCoord(-this->X, -this->Y);
}


FCoordPair::FCoordPair()
{
	A = FCoord();
	B = FCoord();
}

FCoordPair::FCoordPair(const FCoord InA, const FCoord InB)
{
	if (InA < InB)
	{
		A = InA;
		B = InB;
	} else
	{
		A = InB;
		B = InA;
	}
}
	
bool FCoordPair::operator== (const FCoordPair& Other) const
{
	// First check parallel equals
	if ((A == Other.A) && (B == Other.B))
	{
		return true;
	}
	// Second check cross equals
	if ((A == Other.B) && (B == Other.A))
	{
		return true;
	}
	return false;
}


FDungeonRoom::FDungeonRoom()
{
	GlobalCentre = FCoord();
	LocalCoordOffsets = {};
	PossibleRoomsIndex = -1;
}

FDungeonRoom::FDungeonRoom(const FCoord InGlobalCentre, const TArray<FCoord>& InLocalCoordOffsets)
{
	GlobalCentre = InGlobalCentre;
	LocalCoordOffsets = InLocalCoordOffsets;
	PossibleRoomsIndex = -1;
}
	
FDungeonRoom::FDungeonRoom(const FCoord InGlobalCentre, const TArray<FCoord>& InLocalCoordOffsets, const int InPossibleRoomsIndex)
{
	GlobalCentre = InGlobalCentre;
	LocalCoordOffsets = InLocalCoordOffsets;
	PossibleRoomsIndex = InPossibleRoomsIndex;
}

int FDungeonRoom::MaxManhattanDistanceBetweenRooms(TArray<FCoord> A, TArray<FCoord> B)
{
	int MaxA = 0;
	for (const FCoord C : A)
	{
		MaxA = FMath::Max3(MaxA,abs(C.X),abs(C.Y));
	}
	int MaxB = 0;
	for (const FCoord C : B)
	{
		MaxB = FMath::Max3(MaxB,abs(C.X),abs(C.Y));
	}
	return 2 * (MaxA + MaxB + 1);
}

bool FDungeonRoom::DoRoomsOverlap(FDungeonRoom A, FDungeonRoom B)
{		
	TSet<FCoord> GlobalCoords;
	for (FCoord Coord : A.LocalCoordOffsets)
	{
		GlobalCoords.Add(Coord+A.GlobalCentre);
	}
	bool WasInSet = false;
	for (FCoord Coord : B.LocalCoordOffsets)
	{
		GlobalCoords.Add(Coord+B.GlobalCentre, &WasInSet);
		if (WasInSet) return true;
	}
	return false;
}

bool FDungeonRoom::AreRoomsTouching(const FDungeonRoom& A, const FDungeonRoom& B)
{
	if (DoRoomsOverlap(A, B))
	{
		return false;
	}
	// Make sure that if one of the rooms expands by 1, they do overlap
	// Expand room A by 1
	const TSet<FCoord> ReferenceLocalCoordsA = TSet(A.LocalCoordOffsets); 
	TSet<FCoord> LocalCoordsA;
	// For every coord, add all adjacent coords to set
	for (const FCoord CurrentCoord : A.LocalCoordOffsets)
	{
		for (FCoord AdjacentCoord : FCoord::Get4AdjacentTiles(CurrentCoord))
		{
			if (ReferenceLocalCoordsA.Contains(AdjacentCoord))
			{
				continue;
			}
			LocalCoordsA.Add(AdjacentCoord);
		}	
	}
		
	FDungeonRoom ExpandedA = A;
	ExpandedA.LocalCoordOffsets = LocalCoordsA.Array();
	return DoRoomsOverlap(ExpandedA, B);
}



void ADungeonGenerator::GenerateLayout(const int NumRooms)
{
	// First check that num of rooms is valid
	if ((NumRooms <= 0) || (NumRooms > 100)) // TODO make max more if it is efficient
	{
		UE_LOG(LogTemp, Error, TEXT("NumRooms out of bounds (%d). Should be between 0 and 20. Exiting..."), NumRooms);
		return;
	}

	// Populate PotentialRooms with some layouts (just squares and rectangles for now)
	TArray<TArray<FCoord>> PossibleRooms = InitPossibleRooms();

	// Calculate every combination of two rooms.
	const FDateTime StartTime = FDateTime::UtcNow();
	const TArray<TArray<TArray<FCoord>>> RoomComboOffsets = GenerateRoomComboOffsets(PossibleRooms);
	const float TimeElapsedInMs = (FDateTime::UtcNow() - StartTime).GetTotalMilliseconds();
	UE_LOG(LogTemp, Display, TEXT("Generated Room Offsets in %fms"), TimeElapsedInMs)
	
	TArray<FDungeonRoom> RoomLayout = {};
	TSet<FCoord> RoomLayoutUsedCoords;
	int HardCodedRoom1Index = 0;
	RoomLayout.Add({FCoord(0,0), PossibleRooms[HardCodedRoom1Index], HardCodedRoom1Index}); // Same as recursive case but allows for hard coding starter room or something
	for (FCoord Coord : PossibleRooms[HardCodedRoom1Index])
	{
		RoomLayoutUsedCoords.Add(Coord);
	}
	
	// Recursively place rest down
	for (int i = 2; i <= NumRooms; i++)
	{
		AddSingleRoomToLayout(RoomComboOffsets, PossibleRooms, RoomLayout, RoomLayoutUsedCoords);
	}

	SpawnMeshes(RoomLayout);
}

TArray<TArray<FCoord>> ADungeonGenerator::InitPossibleRooms() const
{
	// Init
	TArray<TArray<FCoord>> AlLRooms;;

	// AlLRooms.Add({
	// 	FCoord(-1,-1),
	// 	FCoord(-1,0),
	// 	FCoord(-1,1),
	// 	FCoord(0,-1),
	// 	FCoord(0,0),
	// 	FCoord(0,1),
	// 	FCoord(1,-1),
	// 	FCoord(1,0),
	// 	FCoord(1,1),
	// });
	// AlLRooms.Add({
	// 	FCoord(-2,-1),
	// 	FCoord(-2,0),
	// 	FCoord(-2,1),
	// 	FCoord(-1,-1),
	// 	FCoord(-1,0),
	// 	FCoord(-1,1),
	// 	FCoord(0,-1),
	// 	FCoord(0,0),
	// 	FCoord(0,1),
	// 	FCoord(1,-1),
	// 	FCoord(1,0),
	// 	FCoord(1,1),
	// 	FCoord(2,-1),
	// 	FCoord(2,0),
	// 	FCoord(2,1),
	// });
	
	
	//Iterate over every rectangle between 3 and 6 tiles dimension
	 constexpr int MinSize = 2;
	 constexpr int MaxSize = 3;
	 for (int Width = MinSize; Width <= MaxSize; Width++)
	 {
	 	for (int Height = MinSize; Height <= MaxSize; Height++)
	 	{
	 		int W_Pos, W_Neg, H_Pos, H_Neg;
	 		if (Width % 2 == 0)
	 		{
	 			W_Pos = Width/2;
	 			W_Neg = -Width/2 + 1;
	 		} else
	 		{
	 			W_Pos = Width/2;
	 			W_Neg = -Width/2;
	 		}
	 		if (Height % 2 == 0)
	 		{
	 			H_Pos = Height/2 + 1;
	 			H_Neg = -Height/2;
	 		} else
	 		{
	 			H_Pos = Height/2;
	 			H_Neg = -Height/2;
	 		}
	
	 		TArray<FCoord> RoomOffsetLayout;
	 		for (int W = W_Neg; W <= W_Pos; W++)
	 		{
	 			for (int H = H_Neg; H <= H_Pos; H++)
	 			{
	 				RoomOffsetLayout.Add(FCoord(W,H));
	 			}
	 		}
	 		AlLRooms.Add(RoomOffsetLayout);
	 		
	 	}
	 }


	UE_LOG(LogTemp, Warning, TEXT("Total generated possible rooms: %d"), AlLRooms.Num());

	AlLRooms.Sort([this](const TArray<FCoord>& Item1, const TArray<FCoord>& Item2) {
		return FMath::FRand() < 0.5f;
	});

	TArray<TArray<FCoord>> PossibleRooms = {};

	// Initialise max possible rooms, and reduces if not enough rooms in AllRooms
	int MaxRoomsInGen = 12;
	MaxRoomsInGen = FMath::Min(MaxRoomsInGen, AlLRooms.Num());

	// Adds a random selection of 10 rooms into generation
	for (int i = 0; i < MaxRoomsInGen; i++)
	{
		PossibleRooms.Add(AlLRooms[i]);
	}
	UE_LOG(LogTemp, Warning, TEXT("Total sampled possible rooms for actual generation: %d"), PossibleRooms.Num());
	return PossibleRooms;
}

// Finds every offset to play Room B next to Room A
TArray<FCoord> ADungeonGenerator::GenerateOffsetsForRooms(const TArray<FCoord>& RoomA, const TArray<FCoord>& RoomB)
{	
	TArray<FCoord> OutArray;

	const int MaxBFSRange = FDungeonRoom::MaxManhattanDistanceBetweenRooms(RoomA, RoomB);

	const FDungeonRoom RoomADungeonRoom = FDungeonRoom(FCoord(), RoomA);
	FDungeonRoom RoomBDungeonRoom = FDungeonRoom(FCoord(), RoomB);

	TSet<FCoord> VisitedCoords;
	TQueue<FCoord> SearchQueue;
	SearchQueue.Enqueue(FCoord(0,0));
	VisitedCoords.Add(FCoord(0,0));

	const TSet<FCoord> RoomACoords = TSet(RoomA);
	int CurrentRange = 0;
	while (CurrentRange <= MaxBFSRange)
	{
		// Take top item from queue
		FCoord CurrentCoord;
		SearchQueue.Dequeue(CurrentCoord);

		// If current coord not in RoomA
		if (!RoomACoords.Contains(CurrentCoord))
		{
			// Process it
			CurrentRange = CurrentCoord.X + CurrentCoord.Y;
			RoomBDungeonRoom.GlobalCentre = CurrentCoord;

			if (FDungeonRoom::AreRoomsTouching(RoomADungeonRoom, RoomBDungeonRoom))
			{
				OutArray.Add(CurrentCoord);
			}
		}

		// Add all neighbors to queue
		for (FCoord AdjacentCoord : FCoord::Get4AdjacentTiles(CurrentCoord))
		{
			if (!VisitedCoords.Contains(AdjacentCoord))
			{
				VisitedCoords.Add(AdjacentCoord);
				SearchQueue.Enqueue(AdjacentCoord);
			}
		}		
	}
	return OutArray;
}

TArray<TArray<TArray<FCoord>>> ADungeonGenerator::GenerateRoomComboOffsets(TArray<TArray<FCoord>> PossibleRooms)
{
	TArray<TArray<TArray<FCoord>>> RoomComboOffsets = {};

	for (int i = 0; i < PossibleRooms.Num(); i++)
	{
		TArray<TArray<FCoord>> SecondRoomConnections;
		SecondRoomConnections.Init(TArray<FCoord>(), PossibleRooms.Num());
		RoomComboOffsets.Add(SecondRoomConnections);
	}

	
	
	// Iterates over every room, and adds coord offsets from one room to the other
	for (int i = 0; i < PossibleRooms.Num(); i++)
	{
		for (int j = i; j < PossibleRooms.Num(); j++)
		{
			TArray<FCoord> ItoJOffsets = GenerateOffsetsForRooms(PossibleRooms[i], PossibleRooms[j]);
			RoomComboOffsets[i][j] = ItoJOffsets;

			// If the Room is for itself, do reverse
			if (i == j)
			{
				RoomComboOffsets[j][i] = ItoJOffsets;
				continue;
			}

			// Add inverse coords for inverse rooms
			TArray<FCoord> JtoIOffsets;
			for (FCoord Offset : ItoJOffsets)
			{
				JtoIOffsets.Add(Offset.Inverse());
			}
			RoomComboOffsets[j][i] = JtoIOffsets;
		}
	}
	return RoomComboOffsets;
	
}

void ADungeonGenerator::AddSingleRoomToLayout(TArray<TArray<TArray<FCoord>>> RoomComboOffsets, TArray<TArray<FCoord>> PossibleRooms, TArray<FDungeonRoom> &RoomLayout, TSet<FCoord> &RoomLayoutUsedCoords) const
{
	// Take a new random room layout (will be RoomB, placing room)
	const int NewRoomIndex = FMath::RandRange(0,PossibleRooms.Num()-1);
	TArray<FCoord> NewRoomLocalCoords = PossibleRooms[NewRoomIndex];
	//UE_LOG(LogTemp, Warning, TEXT("Selected Room %d to be placed."), NewRoomIndex)
	

	// Find every existing room and their PossibleRooms index
	// Find max manhattan distance across all placed rooms, and the random room layout
	TSet<FCoord> PlaceableLocations;
	for (FDungeonRoom Room : RoomLayout)
	{
		// Find all placeable points -> filter out direct centre overlaps
		for (FCoord PossibleLocationOffset : RoomComboOffsets[Room.PossibleRoomsIndex][NewRoomIndex])
		{
			FCoord PossibleLocation = Room.GlobalCentre + PossibleLocationOffset;

			// Get Global Coords for this hypothetical dungeon
			bool CanUse = true;
			for (FCoord LocalOffsetFromPotentialCentre : NewRoomLocalCoords)
			{
				FCoord PotentialGlobalCoord = LocalOffsetFromPotentialCentre + PossibleLocation;
				if (RoomLayoutUsedCoords.Contains(PotentialGlobalCoord))
				{
					CanUse = false;
				}
			}
			if (CanUse)
			{
				PlaceableLocations.Add(PossibleLocation);
			}
			
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("After filtering out locations, %d possible room origins remain."), PlaceableLocations.Num())

	// Sort these locations by EUCLIDEAN distance from 0,0
	TArray<FCoord> ListOfSpawnLocations =  PlaceableLocations.Array();
	// Shuffles so order of added locations do not matter

	// TODO make choice of position dependant on input - also change size of room selected
	ListOfSpawnLocations.Sort([this](const FCoord &Item1, const FCoord &Item2) {
		return FMath::FRand() < 0.5f;
	});

	// Sort to get closest first
	// ListOfSpawnLocations.Sort();

	// UE_LOG(LogTemp, Error, TEXT("Found %d possible room locations!"), ListOfSpawnLocations.Num())
	
	// Perform weighted random selection
	// Temp implementation just picks closest
	const FCoord RoomCentre = ListOfSpawnLocations[0];
	
	// Place new random room layout in new location, and print coords
	FDungeonRoom NewRoom = FDungeonRoom(RoomCentre, NewRoomLocalCoords, NewRoomIndex);
	RoomLayout.Add(NewRoom);
	
	// Update global set of coord tiles
	for (FCoord LocalCoord : NewRoom.LocalCoordOffsets)
	{
		RoomLayoutUsedCoords.Add(LocalCoord+NewRoom.GlobalCentre);
	}
}


void ADungeonGenerator::SpawnMeshes(const TArray<FDungeonRoom>& RoomLayout)
{
	TMap<FCoord, int> TilesUsed;
	
	// For every tile of every room, spawn a floor tile
	for (int RoomIndex = 0; RoomIndex < RoomLayout.Num(); RoomIndex++)
	{
		FDungeonRoom Room = RoomLayout[RoomIndex];
		for (const FCoord LocalOffset : Room.LocalCoordOffsets)
		{
			TilesUsed.Add(Room.GlobalCentre+LocalOffset, RoomIndex);
			
			const int X = Room.GlobalCentre.X + LocalOffset.X;
			const int Y = Room.GlobalCentre.Y + LocalOffset.Y;
			FVector SpawnLocation = FVector(X*FloorMeshWidth, Y*FloorMeshWidth, 0);
			
			// Doing by spawning actors
			AActor* NewTile = GetWorld()->SpawnActor<AActor>(FloorActor, SpawnLocation, FRotator(0, 0, 0));
			//AStaticMeshActor* NewMesh = GetWorld()->SpawnActor<AStaticMeshActor>(SpawnLocation, FRotator(0, 0, 0));
			//NewMesh->GetStaticMeshComponent()->SetStaticMesh(FloorMesh);
	
			FloorActors.Push(NewTile);
		}
	}


	// Initialise a graph for the room layout, and connections between rooms calculated when finding wall coordinates
	TArray<TArray<TSet<FCoordPair>>> AdjacencyMap;
	for (int i = 0; i < RoomLayout.Num(); i++)
	{
		TArray<TSet<FCoordPair>> Connections;
		Connections.Init(TSet<FCoordPair>(), RoomLayout.Num());
		AdjacencyMap.Add(Connections);
	}
	
	
	// Find unique wall coordinates
	TSet<FCoordPair> WallLocations;
	for (FDungeonRoom Room : RoomLayout)
	{
		// For every tile in the room, find all adjacent tiles that are NOT in the room.
		for (FCoord LocalOffset : Room.LocalCoordOffsets)
		{
			for (FCoord AdjacentTile : FCoord::Get4AdjacentTiles(LocalOffset))
			{
				// If the tile is in the room, then skip it
				if (Room.LocalCoordOffsets.Contains(AdjacentTile)) { continue; }

				// Add the wall to the wall coordinates
				const FCoord ThisTile = FCoord(Room.GlobalCentre + LocalOffset);
				const FCoord NeighborTile = FCoord(Room.GlobalCentre + AdjacentTile);
				WallLocations.Add(FCoordPair(ThisTile, NeighborTile));

				// If the wall is between two rooms, add a connection between those rooms in the map graph,
				// and store this coordinate as one of the connecting walls
				if (TilesUsed.Contains(NeighborTile))
				{
					int Tile1RoomIndex = TilesUsed[ThisTile];
					int Tile2RoomIndex = TilesUsed[NeighborTile];
					if (Tile2RoomIndex < Tile1RoomIndex)
					{
						// Swapped to ensure consistent ordering for comparisons (E.g. since A->B is not equal to B->A, we want to sort alphabetically so any B-> turns into A->B to add to the set)
						Swap(Tile1RoomIndex, Tile2RoomIndex);
					}
					AdjacencyMap[Tile1RoomIndex][Tile2RoomIndex].Add(FCoordPair(ThisTile, NeighborTile));
				}
			}
		}
	}


	// Find all the rooms connections.
	// For every room connection, take a random coord out of the wall set, and place it in an archway set
	// Spawn archways in every FCoordPair in this set
	for (int FromRoomIndex = 0; FromRoomIndex < RoomLayout.Num(); FromRoomIndex++)
	{
		for (int ToRoomIndex = 0; ToRoomIndex < RoomLayout.Num(); ToRoomIndex++)
		{
			// If no connections, continue
			if (AdjacencyMap[FromRoomIndex][ToRoomIndex].Num() == 0) { continue; }

			TArray<FCoordPair> ConnectingWalls = AdjacencyMap[FromRoomIndex][ToRoomIndex].Array();
			FCoordPair RandomWallConnection = ConnectingWalls[FMath::RandRange(0,ConnectingWalls.Num()-1)];

			// Remove from wall connections
			WallLocations.Remove(RandomWallConnection);
		}
	}
	
	// Spawn in wall meshes at each wall coordinate
	for (const FCoordPair Location : WallLocations)
	{
		SpawnWall(Location);
	}
}

void ADungeonGenerator::SpawnWall(const FCoordPair Location)
{
	const float X = (Location.A.X + Location.B.X)/2.f;
	const float Y = (Location.A.Y + Location.B.Y)/2.f;
	FRotator SpawnRotation;
	// If X unchanged, spawn on biggest Y val coord, otherwise biggest X val coord
	if (Location.A.X == Location.B.X) {
		// X = Location.A.X;
		// Y = FMath::Max(Location.A.Y, Location.B.Y);
		SpawnRotation = FRotator(0,0,0);
	} else
	{
		// X = FMath::Max(Location.A.X, Location.B.X);
		// Y = Location.A.Y;
		// should be diff rotation
		SpawnRotation = FRotator(0,90,0);
	}

	const FVector SpawnLocation = FVector(X*FloorMeshWidth, Y*FloorMeshWidth, 0);
	// Doing by spawning actors
	AActor* NewMesh = GetWorld()->SpawnActor<AActor>(WallActor, SpawnLocation, SpawnRotation);
	// AStaticMeshActor* NewMesh = GetWorld()->SpawnActor<AStaticMeshActor>(SpawnLocation, SpawnRotation);
	// NewMesh->GetStaticMeshComponent()->SetStaticMesh(Mesh);
	
	WallActors.Push(NewMesh);
}
