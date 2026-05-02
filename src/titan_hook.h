#pragma once
#include <Windows.h>
#include <detours.h>
#pragma comment(lib, "detours.lib")
template <typename T>
class TitanHook {
public:
	TitanHook() = default;

	void InitHook(void* targetFunc, void* myFunc) {
		targetFunc_ = targetFunc;
		myFunc_ = myFunc;
		installed_ = false;
	}

	bool SetHook() {
		if (targetFunc_ == nullptr || myFunc_ == nullptr || installed_) {
			return installed_;
		}
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourAttach(&(LPVOID&)targetFunc_, myFunc_);
		installed_ = DetourTransactionCommit() == NO_ERROR;
		return installed_;
	}

	T GetOrignalFunc() {
		return (T)targetFunc_;
	}

	void RemoveHook() {
		if (targetFunc_ == nullptr || myFunc_ == nullptr || !installed_) {
			return;
		}
		DetourTransactionBegin();
		DetourUpdateThread(GetCurrentThread());
		DetourDetach(&(LPVOID&)targetFunc_, myFunc_);
		if (DetourTransactionCommit() == NO_ERROR) {
			installed_ = false;
		}
	}

	~TitanHook() {
		RemoveHook();
	}
private:
	void* targetFunc_ = nullptr;
	void* myFunc_ = nullptr;
	bool installed_ = false;
};
