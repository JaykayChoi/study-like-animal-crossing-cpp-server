﻿#include "world.h"
#include "actor/actor.h"

/////////////////////////////////////////////////////////////////////////////////////////
// World::TickThread

World::TickThread::TickThread(World& world)
	: Super("WorldTickThread")
	, world_(world)
{
}

void World::TickThread::Execute()
{
	auto lastTime = std::chrono::steady_clock::now();
	static const auto msPerTick = std::chrono::milliseconds(50);

	while (!bShouldTerminate_)
	{
		auto nowTime = std::chrono::steady_clock::now();
		auto delta = std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastTime).count();
		world_.Tick(static_cast<float>(delta));
		auto tickTime = std::chrono::steady_clock::now() - nowTime;

		if (tickTime < msPerTick)
		{
			// 최소 msPerTick이 될 때 까지 sleep
			std::this_thread::sleep_for(msPerTick - tickTime);
		}

		lastTime = nowTime;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// World

World::World()
	: tickThread_(*this)
{

}

World::~World()
{

}

void World::Start()
{
	tickThread_.Start();
}

void World::Stop()
{
	tickThread_.Stop();
}

void World::AddActor(std::unique_ptr<Actor> actor)
{
	CSLock lock(csActors_);
	actors_.emplace_back(std::move(actor));
}

void World::Tick(float delta)
{
	for (const auto& actor : actors_)
	{
		// TODO tick 필요한 액터 구분 필요.
		actor->Tick(delta);
	}
}
