#include <Ice/Communicator.h>
#include <Ice/Config.h>
#include <Ice/Current.h>
#include <Ice/InputStream.h>
#include <Ice/OutputStream.h>
#include <Ice/Properties.h>
#include <Ice/PropertiesF.h>
#include <boost/lexical_cast.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <cerrno>
#include <core.h>
#include <cstring>
#include <ctime>
#include <exception>
#include <factory.impl.h>
#include <fileUtils.h>
#include <memory>
#include <session.h>
#include <string>
#include <string_view>
#include <sys.h>
#include <sys/file.h>
#include <unistd.h>
#include <utility>

namespace IceSpider {
	class FileSessions : public Plugin, public SessionManager {
	public:
		FileSessions(Ice::CommunicatorPtr com, const Ice::PropertiesPtr & props) :
			ic(std::move(com)), root(props->getProperty("IceSpider.FileSessions.Path")),
			duration(static_cast<Ice::Short>(
					props->getPropertyAsIntWithDefault("IceSpider.FileSessions.Duration", 3600)))
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
			catch (...) { // NOLINT(bugprone-empty-catch) - Meh :)
			}
		}

		void operator=(const FileSessions &) = delete;
		void operator=(FileSessions &&) = delete;

		SessionPtr
		createSession(const ::Ice::Current &) override
		{
			auto session = std::make_shared<Session>();
			// NOLINTNEXTLINE(clang-analyzer-optin.cplusplus.VirtualCall)
			session->id = boost::lexical_cast<std::string>(boost::uuids::random_generator()());
			session->duration = duration;
			save(session);
			return session;
		}

		SessionPtr
		getSession(const ::std::string sessionId, const ::Ice::Current & current) override
		{
			auto session = load(sessionId);
			if (session && isExpired(session)) {
				destroySession(sessionId, current);
				return nullptr;
			}
			return session;
		}

		void
		updateSession(const SessionPtr session, const ::Ice::Current &) override
		{
			save(session);
		}

		void
		destroySession(const ::std::string sessionId, const ::Ice::Current &) override
		{
			try {
				std::filesystem::remove(root / sessionId);
			}
			catch (const std::exception & e) {
				throw SessionError(e.what());
			}
		}

	private:
		void
		save(const SessionPtr & session)
		{
			session->lastUsed = time(nullptr);
			Ice::OutputStream buf(ic);
			buf.write(session);
			const auto range = buf.finished();
			// NOLINTNEXTLINE(hicpp-signed-bitwise)
			AdHoc::FileUtils::FileHandle sessionFile(root / session->id, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
			sysassert(flock(sessionFile.fh, LOCK_EX), -1);
			sysassert(pwrite(sessionFile.fh, range.first, static_cast<size_t>(range.second - range.first), 0), -1);
			sysassert(ftruncate(sessionFile.fh, range.second - range.first), -1);
			sysassert(flock(sessionFile.fh, LOCK_UN), -1);
		}

		SessionPtr
		load(const std::string & sessionId)
		{
			auto path = root / sessionId;
			if (!std::filesystem::exists(path)) {
				return nullptr;
			}
			try {
				AdHoc::FileUtils::MemMap sessionFile(path);
				sysassert(flock(sessionFile.fh, LOCK_SH), -1);
				auto fbuf = sessionFile.sv<Ice::Byte>();
				Ice::InputStream buf(ic, std::make_pair(fbuf.begin(), fbuf.end()));
				SessionPtr session;
				buf.read(session);
				sysassert(flock(sessionFile.fh, LOCK_UN), -1);
				return session;
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
			std::filesystem::directory_iterator dirIter(root);
			while (dirIter != std::filesystem::directory_iterator()) {
				auto session = load(dirIter->path());
				if (session && isExpired(session)) {
					FileSessions::destroySession(session->id, Ice::Current());
				}
				dirIter++;
			}
		}

		[[nodiscard]]
		static bool
		isExpired(const SessionPtr & session)
		{
			return (session->lastUsed + session->duration < time(nullptr));
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
		const Ice::Short duration;
	};
}

NAMEDFACTORY("IceSpider-FileSessions", IceSpider::FileSessions, IceSpider::PluginFactory);
