#ifndef N6_MODULE_MOUNT_WORKER_H
#define N6_MODULE_MOUNT_WORKER_H

#include <cerrno>
#include <cstdlib>
#include <pthread.h>
#include <sched.h>
#include <sys/mount.h>

namespace n6_module_mount {

// Keep the native loader synchronous and in-process so its property and
// Recovery state changes still apply. Only the mount view is per-thread.
inline int Run(bool (*load)(), bool& loaded) {
	struct Job {
		bool (*load)();
		int error;
		bool loaded;
	} job{load, 0, false};

	pthread_t worker;
	int rc = pthread_create(&worker, nullptr, [](void* arg) -> void* {
		auto& job = *static_cast<Job*>(arg);
		if (unshare(CLONE_NEWNS) < 0 ||
		    mount(nullptr, "/", nullptr, MS_REC | MS_PRIVATE, nullptr) < 0) {
			job.error = errno;
			return nullptr;
		}
		job.loaded = job.load();
		return nullptr;
	}, &job);
	if (rc != 0)
		return rc;
	// This joinable thread has exactly one joiner. Never return while it
	// could still be using the caller's Job or Recovery partition state.
	if (pthread_join(worker, nullptr) != 0)
		std::abort();
	loaded = job.loaded;
	return job.error;
}

} // namespace n6_module_mount

#endif
