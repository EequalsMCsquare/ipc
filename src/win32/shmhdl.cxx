#include <Windows.h>
#include <system_error>
#include <atomic>

#include "shmhdl.hpp"
#include "ec.hpp"

namespace ipc
{
	shmhdl::shmhdl(std::string_view name, const shmsz_t nbytes, std::error_code& ec) noexcept
	{
		this->hMapFile_ = nullptr;
		ec.clear();

		// calculate required bytes
		ULARGE_INTEGER __nbytes;
		size_t __reqbytes = nbytes + sizeof(shm_meta_t);
		CopyMemory(&__nbytes, &__reqbytes, sizeof(uint64_t));

		HANDLE __hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, __nbytes.HighPart, __nbytes.LowPart, name.data());
		// Win32 API, even the name is used by existing shared memory object,
		// CreateFileMapping with the exact same name will still return a valid
		// HANDLE... This is wired.
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			CloseHandle(__hMapFile);
			ec.assign(ERROR_ALREADY_EXISTS, std::system_category());
			return;
		}
		if (__hMapFile == nullptr) {
			ec.assign(GetLastError(), std::system_category());
			return;
		}

		// setup shmhdl
		void* __meta = MapViewOfFile(__hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shm_meta_t));
		if (__meta == nullptr) {
			ec.assign(GetLastError(), std::system_category());
			CloseHandle(__hMapFile);
			return;
		}
		// success
		this->meta_ = new(__meta) shm_meta_t;
		this->meta_->ref_count_ = 1;
		this->meta_->status_ = SHM_STATUS::OK;
		this->meta_->shmsz_ = nbytes;

		this->hMapFile_ = __hMapFile;
		this->name_ = { name.begin(), name.end() };

		this->addr_ = nullptr;
	}
	shmhdl::shmhdl(std::string_view name, const shmsz_t nbytes) {
		this->hMapFile_ = nullptr;
		std::error_code ec;

		// calculate required bytes
		ULARGE_INTEGER __nbytes;
		size_t __reqbytes = nbytes + sizeof(shm_meta_t);
		CopyMemory(&__nbytes, &__reqbytes, sizeof(uint64_t));

		HANDLE __hMapFile = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, __nbytes.HighPart, __nbytes.LowPart, name.data());
		// Win32 API, even the name is used by existing shared memory object,
		// CreateFileMapping with the exact same name will still return a valid
		// HANDLE... This is wired.
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			CloseHandle(__hMapFile);
			ec.assign(ERROR_ALREADY_EXISTS, std::system_category());
			throw std::system_error(ec);
		}
		if (__hMapFile == nullptr) {
			ec.assign(GetLastError(), std::system_category());
			return;
		}

		// setup shmhdl
		void* __meta = MapViewOfFile(__hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shm_meta_t));
		if (__meta == nullptr) {
			ec.assign(GetLastError(), std::system_category());
			CloseHandle(__hMapFile);
			throw std::system_error(ec);
		}
		// success
		this->meta_ = new(__meta) shm_meta_t;
		this->meta_->ref_count_ = 1;
		this->meta_->status_ = SHM_STATUS::OK;
		this->meta_->shmsz_ = nbytes;

		this->hMapFile_ = __hMapFile;
		this->name_ = { name.begin(), name.end() };

		this->addr_ = nullptr;
	}

	shmhdl::shmhdl(std::string_view name, std::error_code& ec) noexcept {
		this->hMapFile_ = nullptr;
		ec.clear();
		// try to open shmhdl
		HANDLE __hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, true, name.data());
		if (__hMapFile == nullptr) {
			ec.assign(GetLastError(), std::system_category());
			return;
		}

		// map shm_meta
		LPVOID __meta = MapViewOfFile(__hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shm_meta_t));
		// fail
		if (__meta == nullptr) {
			ec.assign(GetLastError(), std::system_category());
			CloseHandle(__hMapFile);
			return;
		}
		// map success -> check shmhdl STATUS
		this->meta_ = reinterpret_cast<shm_meta_t*>(__meta);
		if (meta_->status_ == SHM_STATUS::DEL) {
			UnmapViewOfFile(__meta);
			CloseHandle(__hMapFile);
			this->hMapFile_ = nullptr;
			ec = IPCErrc::ShmDeleted;
			return;
		}
		this->meta_->ref_count_++;

		// setup local var
		this->hMapFile_ = __hMapFile;
		this->name_ = { name.begin(), name.end() };
		this->addr_ = nullptr;
	}

	shmhdl::shmhdl(std::string_view name)
	{
		this->hMapFile_ = nullptr;
		std::error_code ec;
		// try to open shmhdl
		HANDLE __hMapFile = OpenFileMappingA(FILE_MAP_ALL_ACCESS, true, name.data());
		if (__hMapFile == nullptr) {
			ec.assign(GetLastError(), std::system_category());
			throw std::system_error(ec);
		}

		// map shm_meta
		LPVOID __meta = MapViewOfFile(__hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(shm_meta_t));
		// fail
		if (__meta == nullptr) {
			ec.assign(GetLastError(), std::system_category());
			CloseHandle(__hMapFile);
			throw std::system_error(ec);
		}
		// map success -> check shmhdl STATUS
		this->meta_ = reinterpret_cast<shm_meta_t*>(__meta);
		if (meta_->status_ == SHM_STATUS::DEL) {
			UnmapViewOfFile(__meta);
			CloseHandle(__hMapFile);
			ec = IPCErrc::ShmDeleted;
			throw std::system_error(ec);
		}
		this->meta_->ref_count_++;

		// setup local var
		this->hMapFile_ = __hMapFile;
		this->name_ = { name.begin(), name.end() };
		this->addr_ = nullptr;
	}

	shmhdl::~shmhdl()
	{
		std::error_code ec;
		if (this->hMapFile_ != nullptr) {
			this->meta_->ref_count_ -= 1;
			if (this->meta_->ref_count_ == 0) {
				this->meta_->status_ = SHM_STATUS::DEL;
				this->unmap(ec);
				this->unmap_meta(ec);
				CloseHandle(hMapFile_);
				hMapFile_ = nullptr;
				return;
			}
			this->unmap(ec);
			this->unmap_meta(ec);
			CloseHandle(hMapFile_);
			hMapFile_ = nullptr;
		}
	}

	void* shmhdl::map(std::error_code& ec) noexcept {
		ec.clear();
		if (this->addr_ == nullptr) {
			void* __ptr = MapViewOfFile(hMapFile_, FILE_MAP_ALL_ACCESS, 0, 0, 0);
			// fail
			if (__ptr == nullptr) {
				ec.assign(GetLastError(), std::system_category());
				return nullptr;
			}
			// success
			this->addr_ = reinterpret_cast<char*>(__ptr) + sizeof(shm_meta_t);
			return this->addr_;

		}
		return this->addr_;
	}

	void* shmhdl::map() {
		std::error_code ec;
		void* __ptr = this->map(ec);
		if (ec) {
			throw std::system_error(ec);
		}
		return __ptr;
	}

	void shmhdl::unmap_meta(std::error_code& ec) noexcept
	{
		ec.clear();
		if (this->meta_ == nullptr) {
			ec = IPCErrc::ShmAddrNullptr;
			return;
		}
		if (UnmapViewOfFile(this->meta_) == 0) {
			ec.assign(GetLastError(), std::system_category());
			return;
		}
		this->meta_ = nullptr;
	}

	void shmhdl::unmap(std::error_code& ec) noexcept {
		ec.clear();
		if (this->addr_) {
			if (UnmapViewOfFile(reinterpret_cast<char*>(this->addr_) - sizeof(shm_meta_t)) == 0) {
				ec.assign(GetLastError(), std::system_category());
				return;
			}

			this->addr_ = nullptr;
		}
	}

	void shmhdl::unmap()
	{
		std::error_code ec;
		this->unmap(ec);
		if (ec) {
			throw std::system_error(ec);
		}
	}

	void shmhdl::unlink(std::error_code& ec) noexcept
	{
		ec.clear();
		this->meta_->status_ = SHM_STATUS::DEL;
	}
	void shmhdl::unlink()
	{
		this->meta_->status_ = SHM_STATUS::DEL;
	}

	shmsz_t shmhdl::nbytes() const noexcept {
		return this->meta_->shmsz_;
	}

	std::string_view shmhdl::name() const noexcept
	{
		return this->name_;
	}

	void* shmhdl::addr() const noexcept
	{
		return this->addr_;
	}

	size_t shmhdl::ref_count() const noexcept
	{
		return this->meta_->ref_count_;
	}

	HANDLE shmhdl::native_handle() const noexcept
	{
		return this->hMapFile_;
	}
}
