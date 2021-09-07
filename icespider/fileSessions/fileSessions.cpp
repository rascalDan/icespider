#include "Ice/Initialize.h"
#include <Ice/InputStream.h>
#include <Ice/OutputStream.h>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <core.h>
#include <factory.impl.h>
#include <fcntl.h>
#include <fileUtils.h>
#include <filesystem>
#include <session.h>
#include <sys.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/stat.h>

namespace IceSpider {
	class FileSessions : public Plugin, public SessionManager {
	public:
		FileSessions(Ice::CommunicatorPtr c, const Ice::PropertiesPtr & p) :
			ic(std::move(c)), root(p->getProperty("IceSpider.FileSessions.Path")),
			duration(p->getPropertyAsIntWithDefault("IceSpider.FileSessions.Duration", 3600))
		{
			if (!root.empty() && !std::filesystem::exists(root)) {
				std::filesystem::create_directories(root);
			}
		}

		FileSessions(const FileSessions &) = delete;
		FileSessions(FileSessions &&) = delete;

		~FileSessions() override
		{
			try {
				removeExpired();
			}
			catch (...) {
				// Meh :)
			}
		}

		void operator=(const FileSessions &) = delete;
		void operator=(FileSessions &&) = delete;

		SessionPtr
		createSession(const ::Ice::Current &) override
		{
			auto s = std::make_shared<Session>();
			// NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
			s->id = boost::lexical_cast<std::string>(boost::uuids::random_generator()());
			s->duration = duration;
			save(s);
			return s;
		}

		SessionPtr
		getSession(const ::std::string id, const ::Ice::Current & current) override
		{
			auto s = load(id);
			if (s && isExpired(s)) {
				destroySession(id, current);
				return nullptr;
			}
			return s;
		}

		void
		updateSession(const SessionPtr s, const ::Ice::Current &) override
		{
			save(s);
		}

		void
		destroySession(const ::std::string id, const ::Ice::Current &) override
		{
			try {
				std::filesystem::remove(root / id);
			}
			catch (const std::exception & e) {
				throw SessionError(e.what());
			}
		}

	private:
		void
		save(const SessionPtr & s)
		{
			s->lastUsed = time(nullptr);
			Ice::OutputStream buf(ic);
			buf.write(s);
			auto range = buf.finished();
			// NOLINTNEXTLINE(hicpp-signed-bitwise)
			AdHoc::FileUtils::FileHandle f(root / s->id, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
			sysassert(flock(f.fh, LOCK_EX), -1);
			sysassert(pwrite(f.fh, range.first, range.second - range.first, 0), -1);
			sysassert(ftruncate(f.fh, range.second - range.first), -1);
			sysassert(flock(f.fh, LOCK_UN), -1);
		}

		SessionPtr
		load(const std::string & id)
		{
			auto path = root / id;
			if (!std::filesystem::exists(path)) {
				return nullptr;
			}
			try {
				AdHoc::FileUtils::MemMap f(path);
				sysassert(flock(f.fh, LOCK_SH), -1);
				auto fbuf = f.sv<Ice::Byte>();
				Ice::InputStream buf(ic, std::make_pair(fbuf.begin(), fbuf.end()));
				SessionPtr s;
				buf.read(s);
				sysassert(flock(f.fh, LOCK_UN), -1);
				return s;
			}
			catch (const AdHoc::SystemException & e) {
				if (e.errNo == ENOENT) {
					return nullptr;
				}
				throw;
			}
		}

		void
		removeExpired()
		{
			if (root.empty() || !std::filesystem::exists(root)) {
				return;
			}
			std::filesystem::directory_iterator di(root);
			while (di != std::filesystem::directory_iterator()) {
				auto s = load(di->path());
				if (s && isExpired(s)) {
					FileSessions::destroySession(s->id, Ice::Current());
				}
				di++;
			}
		}

		bool
		isExpired(const SessionPtr & s)
		{
			return (s->lastUsed + s->duration < time(nullptr));
		}

		template<typename R, typename ER>
		R
		sysassert(R rtn, ER ertn)
		{
			if (rtn == ertn) {
				throw SessionError(strerror(errno));
			}
			return rtn;
		}

		Ice::CommunicatorPtr ic;
		const std::filesystem::path root;
		const Ice::Int duration;
	};
}
NAMEDFACTORY("IceSpider-FileSessions", IceSpider::FileSessions, IceSpider::PluginFactory);
