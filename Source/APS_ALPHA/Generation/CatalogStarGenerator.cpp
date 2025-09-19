#include "CatalogStarGenerator.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"

const FString SpectralClasses[] = {TEXT("O"), TEXT("B"), TEXT("A"), TEXT("F"), TEXT("G"), TEXT("K"), TEXT("M")};

// Sets default values
ACatalogStarGenerator::ACatalogStarGenerator()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	HISMComponent = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("HISMComponent"));
	RootComponent = HISMComponent;
}

// Called when the game starts or when spawned
void ACatalogStarGenerator::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Warning, TEXT("CatalogStarGenerator BeginPlay()"));

	// ��������� ��� MaterialSphere ��� HISM-����������
	/*static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshAsset(TEXT("/Engine/�ontent/BasicShapes/Sphere"));
	if (SphereMeshAsset.Succeeded())
	{
		UStaticMesh* SphereMesh = SphereMeshAsset.Object;
		if (SphereMesh)
		{
			HISMComponent->SetStaticMesh(SphereMesh);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to load MaterialSphere mesh"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find MaterialSphere mesh asset"));
	}*/

	// ������ CSV-�����
	FString CSVFilePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/APS_ALPHA/hyg_v35.csv"));
	FString CSVData;

	if (FFileHelper::LoadFileToString(CSVData, *CSVFilePath))
	{
		// ������ CSV-������
		TArray<FString> Rows;
		CSVData.ParseIntoArray(Rows, TEXT("\n"), true);

		// ���������� �� ������ ������ � �������� ������ 10000 �������
		int32 NumStars = 1000; // ������������ ���������� ����� �� 10000
		for (int32 i = 2; i <= NumStars; i++)
		{
			TArray<FString> Columns;
			Rows[i].ParseIntoArray(Columns, TEXT(","));

			if (Columns.Num() < 16) // �������� ��� ����� �� ����������� ���������� �������� � ����� CSV-�����
			{
				continue;
			}


			double RadiusSun = 696340000.0;
			double SphereRadiusDefault = 1000.0; // ������ ����� �� ��������� � ������� �������� (1 ����)
			double ScaleFactor = SphereRadiusDefault / RadiusSun;
			//// �������� ������ � ���������� ������
			double Radius = FCString::Atof(*Columns[15]); // �������� 15 �� ����������� ����� ������� ��� �������
			//double ScaledRadius = Radius * ScaleFactor;
			//double ScaledRadius = Radius * 696340000.0;
			//float X = FCString::Atof(*Columns[17]); // �������� 17 �� ����������� ����� ������� ��� ���������� X
			//float Y = FCString::Atof(*Columns[18]); // �������� 18 �� ����������� ����� ������� ��� ���������� Y
			//float Z = FCString::Atof(*Columns[19]); // �������� 19 �� ����������� ����� ������� ��� ���������� Z

			// �������������� ��������� �� �������� ��� � ������� UE
			double LightYearToUEScale = 9.461e+12; // ����������� �������������� �������� ��� � ������� UE
			double LightYearScale = 100000.0; // ����������� ��������������� � �������� �����

			double X = FCString::Atof(*Columns[17]);
			// * 1000000;// * LightYearToUEScale / LightYearScale; // �������� 17 �� ����������� ����� ������� ��� ���������� X
			double Y = FCString::Atof(*Columns[18]);
			// * 1000000;// * LightYearToUEScale / LightYearScale; // �������� 18 �� ����������� ����� ������� ��� ���������� Y
			double Z = FCString::Atof(*Columns[19]);
			// * 1000000;// * LightYearToUEScale / LightYearScale; // �������� 19 �� ����������� ����� ������� ��� ���������� Z
			//UE_LOG(LogTemp, Warning, TEXT("ScaledRadius: %f"), ScaledRadius);


			// ������� ��������� HISM � ��������� ������������ � ���������
			FVector Location(X, Y, Z); // �������� X, Y, Z �� ����������� �������� ���������
			FTransform InstanceTransform(FQuat::Identity, Location, FVector(Radius));
			HISMComponent->AddInstance(InstanceTransform);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to load CSV file: %s"), *CSVFilePath);
	}


	// read csv file

	// for 10000 first stars

	// get star spectral class, radius and galaxy coordinates 

	// based on spectral class spawn its hism / apply coordinates and radius scale
	// Create HISM for each spectral class
	//for (int32 i = 0; i < 7; i++)
	//{
	//	UHierarchicalInstancedStaticMeshComponent* HISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(this);

	//	// Set up HISM properties here
	//	// HISM->SetStaticMesh();

	//	HISMMap.Add(SpectralClasses[i], HISM);
	//}

	//// Read the CSV file
	//FString CSVFilePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("Source/APS_ALPHA/hyg_v35.csv"));
	//FString CSVData;

	//if (FFileHelper::LoadFileToString(CSVData, *CSVFilePath))
	//{
	//	// Parse the CSV data
	//	TArray<FString> Rows;
	//	CSVData.ParseIntoArray(Rows, TEXT("\n"), true);

	//	// Skip the first row (header)
	//	for (int32 i = 1; i < /*Rows.Num()*/10000; i++)
	//	{
	//		TArray<FString> Columns;
	//		Rows[i].ParseIntoArray(Columns, TEXT(","));

	//		//UE_LOG(LogTemp, Warning, TEXT("Rows[i] = %s"), Rows[i]);
	//		UE_LOG(LogTemp, Warning, TEXT("Rows[%d] = %s"),i, *Rows[i]);

	//		if (Columns.Num() < 16) // Change this number to match the actual number of columns in your CSV file
	//		{
	//			continue;
	//		}

	//		FString SpectralClass = Columns[15]; // Replace 15 with the actual column number for the spectral class
	//		UHierarchicalInstancedStaticMeshComponent* HISM = HISMMap.FindRef(SpectralClass);

	//		if (HISM)
	//		{
	//			// Parse the star's radius and galactic coordinates
	//			float Radius = FCString::Atof(*Columns[2]); // Replace 2 with the actual column number for the radius
	//			FVector GalacticCoordinates = FVector(FCString::Atof(*Columns[3]), FCString::Atof(*Columns[4]), FCString::Atof(*Columns[5])); // Replace 3, 4, 5 with the actual column numbers for the galactic coordinates

	//			// Scale and position the HISM
	//			FVector GalacticCoordinatesF = FVector(GalacticCoordinates.X, GalacticCoordinates.Y, GalacticCoordinates.Z);
	//			HISM->AddInstance(FTransform(FQuat::Identity, GalacticCoordinatesF * 1000, FVector(Radius * 10)));

	//		}
	//	}
	//}
}

// Called every frame
void ACatalogStarGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}
