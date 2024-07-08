#pragma once

class AStarSystem;

class IMovementStrategy
{
public:
	virtual ~IMovementStrategy() = default;

	virtual void ThrustForward(float Value) = 0;
	virtual void ThrustSide(float Value) = 0;
	virtual void ThrustVertical(float Value) = 0;
};

class ImpulseMovementStrategy : public IMovementStrategy
{
public:
	ImpulseMovementStrategy(UStaticMeshComponent* InSpaceshipHull)
		: SpaceshipHull(InSpaceshipHull)
	{
	}

	virtual void ThrustForward(float Value) override;
	virtual void ThrustSide(float Value) override;
	virtual void ThrustVertical(float Value) override;

private:
	UStaticMeshComponent* SpaceshipHull;
};

class OffsetMovementStrategy : public IMovementStrategy
{
public:
	OffsetMovementStrategy(UStaticMeshComponent* InSpaceshipHull)
		: SpaceshipHull(InSpaceshipHull)
	{
	}

	virtual void ThrustForward(float Value) override;
	virtual void ThrustSide(float Value) override;
	virtual void ThrustVertical(float Value) override;

private:
	UStaticMeshComponent* SpaceshipHull;
};

class OffsetWrapMovementStrategy : public IMovementStrategy
{
public:
	OffsetWrapMovementStrategy(UStaticMeshComponent* InSpaceshipHull, AStarSystem* InStarSystem, float InOffsetForce)
		: SpaceshipHull(InSpaceshipHull), StarSystem(InStarSystem), OffsetForce(InOffsetForce)
	{
	}


	virtual void ThrustForward(float Value) override;
	virtual void ThrustSide(float Value) override;
	virtual void ThrustVertical(float Value) override;

private:
	UStaticMeshComponent* SpaceshipHull;
	AStarSystem* StarSystem;
	double OffsetForce;
};
