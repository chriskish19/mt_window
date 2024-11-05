#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include "window_class.hpp"



int WINAPI wWinMain(
	_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nShowCmd
){
	window_create test(L"happy window");

	// Run the main thread message loop.
	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	// check open window count before main thread exits
	// if open window count equals zero it is safe to exit.
	std::unique_lock<std::mutex> lock(test.m_public_mtx_open_window_count);
	bool exit = true;

	while (exit) {

		// main thread checks open window count each time a window is closed
		if (test.get_open_window_count() == 0) {
			// set while loop to condition to false and exit
			exit = false;
		}
		else {
			test.m_public_cv_open_window_count.wait(lock, [&test] { return test.m_public_open_window_count_gate_state.load(); });
		}

		// close the gate and check agian later
		test.m_public_open_window_count_gate_state = false;
	}

	return 0;
}