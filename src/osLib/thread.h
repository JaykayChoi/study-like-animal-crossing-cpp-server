#pragma once

#include "../global.h"
#include "wEvent.h"

// Thread wrapper.
class Thread
{
public:
	Thread(const std::string& threadName);
	virtual ~Thread();

	// 스레드 시작.
	bool Start();

	// bShouldTerminate_ 값을 true 로 변경하고 스레드가 종료 될 때까지 기다린다.
	void Stop();

	// bShouldTerminate_ 변경없이 스레드가 종료 될 때까지 기다린다.
	bool Wait();

    // 이 메서드를 호출하는 스레드가 이 객체에 포함된 스레드인 경우 true 반환.
    bool IsCurrentThread() const;

protected:
	// Thread entrypoint.
	virtual void Execute() = 0;

	// true 일 경우 종료시켜야 됨.
	std::atomic<bool> bShouldTerminate_;

private:
	std::string threadName_;

	std::thread thread_;

	/**
	 * 스레드 객체가 완전히 초기화 될 때까지 스레드 실행을 기다리는데 사용되는 이벤트.
	 * 스레드가 완전히 할당되기 전에 race 가 발생해 IsCurrentThread 호출이 실패하는 것을
	   방지한다.
	 */
	WEvent eventStart_;

	// 스레드 초기화 race 를 방지하기 위해 초기화 이벤트를 기다리는 Execute() wrapper
	void DoExecute();
};
