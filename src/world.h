#pragma once

#include "global.h"
#include "osLib/criticalSection.h"
#include "osLib/thread.h"

class Actor;

class World
{
public:
	World();

	~World();

	void Start();

	void Stop();

	void AddActor(std::unique_ptr<Actor> actor);

private:
	class TickThread : public Thread
	{
		using Super = Thread;

	public:
		TickThread(World& world);

	protected:
		World& world_;

		// Thread overrides.
		virtual void Execute() override;
	};

	TickThread tickThread_;

	CriticalSection csActors_;
	std::vector<std::unique_ptr<Actor>> actors_;

	void Tick(float delta);
};