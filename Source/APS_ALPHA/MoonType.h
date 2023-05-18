#pragma once
UENUM(BlueprintType)
enum class EMoonType : uint8
{
	Rocky				UMETA(DisplayName = "Rocky"), // Скалистые луны (состоят в основном из камня)
	Icy					UMETA(DisplayName = "Icy"), // Ледяные луны (состоят в основном из льда)
	Iron				UMETA(DisplayName = "Iron"), // Железные луны (состоят в основном из металлов)
	Volcanic			UMETA(DisplayName = "Volcanic"), // Вулканические луны (активные вулканы и геотермальная активность)
	Gas					UMETA(DisplayName = "Gas"), // Газовые луны (имеют значительную атмосферу)
	Ocean				UMETA(DisplayName = "Ocean"), // Океанические луны (имеют океаны под поверхностью)
	Continental			UMETA(DisplayName = "Continental"), // Континентальные луны (имеют сушу и океаны)
	Desert				UMETA(DisplayName = "Desert"), // Пустынные луны (мало воды, засушливые условия)
	TidallyLocked		UMETA(DisplayName = "Tidally Locked"), // Приливно заблокированные луны (одна сторона всегда обращена к планете)
	Peculiar			UMETA(DisplayName = "Peculiar"), // Странные луны (необычные свойства, которые не подходят под другие категории)
	CapturedAsteroid	UMETA(DisplayName = "Captured Asteroid") // Захваченные астероиды (астероиды, попавшие на орбиту планеты и ставшие лунами)
};