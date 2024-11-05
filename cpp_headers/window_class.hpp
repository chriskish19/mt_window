#pragma once
#include <Windows.h>
#include <string>
#include "resource.h"
#include <thread>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <chrono>



class window_relative {
public:
	window_relative(std::wstring const& c_name, std::wstring const& title, HINSTANCE hinst,HWND main_handle) noexcept;

	~window_relative() {
		if (m_public_p_running_logic != nullptr) {
			delete m_public_p_running_logic;
		}
	}

	void change_title(std::wstring const& new_title) noexcept;
	const HWND get_window_handle() noexcept { return m_window_handle; }
	const std::thread::id get_id() noexcept { return m_relative_id; }
	void set_thread_p(std::thread* new_pThread) noexcept { m_p_thread = new_pThread; }
	std::thread* get_thread_p() noexcept { return m_p_thread; }

	std::atomic<bool>* m_public_p_running_logic = new std::atomic<bool>(true);
private:
	HWND m_window_handle = nullptr;
	std::thread::id m_relative_id = std::thread::id();
	std::thread* m_p_thread = nullptr;
};




class window_create{
public:
	window_create(std::wstring const& title) noexcept;

	~window_create() {
		// clean up thread objects
		for (auto thread_p : m_thread_mp) {
			delete thread_p.first;
		}
	}

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;

	HWND get_main_window_handle() noexcept { return m_main_window_handle; }

	unsigned int get_open_window_count() noexcept { return m_open_window_count; }

	// gates and latches for winmain function use
	std::mutex m_public_mtx_open_window_count;
	std::condition_variable m_public_cv_open_window_count;
	std::atomic<bool> m_public_open_window_count_gate_state = false;

private:
	LRESULT CALLBACK PrivateWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) noexcept;


	void window_settings() noexcept;
	void create_relative() noexcept;
	void close_relative() noexcept;
	void run_relative_message_pump(std::thread::id this_thread_id) noexcept;
	void run_logic_changes(window_relative* active_open_window) noexcept;
	
	HWND m_main_window_handle = nullptr;
	WNDCLASS m_wc = {};
	HINSTANCE m_hinst = GetModuleHandle(0);
	std::wstring m_c_name = L"Example mt_window";
	std::wstring m_title;
	std::unordered_map<std::thread::id, window_relative*> m_relative_mp = {};
	std::unordered_map<std::thread*, window_relative*> m_thread_mp = {};


	// gates and latches for safely creating a new window
	std::condition_variable m_cv_window_created;
	std::atomic<bool> m_window_created_gate_state = false;

	// gates and latches for safely executing the case statement "case ID_NEWWINDOW_CREATE:"
	std::condition_variable m_cv_window_proc_case_ID_NEWWINDOW_CREATE;
	std::atomic<bool> m_window_proc_case_ID_NEWWINDOW_CREATE = false;

	unsigned int m_open_window_count = 0;  // set to zero because main window doesnt count
};

