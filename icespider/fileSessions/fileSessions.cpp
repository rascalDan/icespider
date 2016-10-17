#include <core.h>
#include <session.h>
#include <fcntl.h>
#include <fileUtils.h>
#include <sys.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <factory.impl.h>
#include <boost/filesystem/operations.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <Ice/Stream.h>
#include "Ice/Initialize.h"

namespace IceSpider {
	class FileSessions : public Plugin, public SessionManager {
		public:
			FileSessions(Ice::CommunicatorPtr c, Ice::PropertiesPtr p) :
				ic(c),
				root(p->getProperty("IceSpider.FileSessions.Path")),
				duration(p->getPropertyAsIntWithDefault("IceSpider.FileSessions.Duration", 3600))
			{
				if (!root.empty())
					if (!boost::filesystem::exists(root))
						boost::filesystem::create_directories(root);
			}

			~FileSessions()
			{
				removeExpired();
			}

			SessionPtr createSession(const ::Ice::Current &) override
			{
				SessionPtr s = new Session();
				s->id = boost::lexical_cast<std::string>(boost::uuids::random_generator()());
				s->duration = duration;
				save(s);
				return s;
			}

			SessionPtr getSession(const ::std::string & id, const ::Ice::Current & current) override
			{
				auto s = load(id);
				if (s && isExpired(s)) {
					destroySession(id, current);
					return NULL;
				}
				return s;
			}

			void updateSession(const SessionPtr & s, const ::Ice::Current &) override
			{
				save(s);
			}

			void destroySession(const ::std::string & id, const ::Ice::Current &) override
			{
				try {
					boost::filesystem::remove(root / id);
				}
				catch (const std::exception & e) {
					throw SessionError(e.what());
				}
			}

		private:
			void save(SessionPtr s)
			{
				time(&s->lastUsed);
				auto buf = Ice::createOutputStream(ic);
				buf->write(s);
				auto range = buf->finished();
				AdHoc::FileUtils::FileHandle f(root / s->id, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
				sysassert(flock(f.fh, LOCK_EX), -1);
				sysassert(pwrite(f.fh, range.first, range.second - range.first, 0), -1);
				sysassert(ftruncate(f.fh, range.second - range.first), -1);
				sysassert(flock(f.fh, LOCK_UN), -1);
			}

			SessionPtr load(const std::string & id)
			{
				auto path = root / id;
				if (!boost::filesystem::exists(path)) return NULL;
				try {
					AdHoc::FileUtils::MemMap f(path);
					sysassert(flock(f.fh, LOCK_SH), -1);
					auto fbuf = (Ice::Byte *)f.data;
					auto buf = Ice::createInputStream(ic, std::make_pair(fbuf, fbuf + f.getStat().st_size));
					SessionPtr s;
					buf->read(s);
					sysassert(flock(f.fh, LOCK_UN), -1);
					return s;
				}
				catch (const AdHoc::SystemException & e) {
					if (e.errNo == ENOENT) {
						return NULL;
					}
					throw;
				}
			}

			void removeExpired()
			{
				if (root.empty() || !boost::filesystem::exists(root)) return;
				boost::filesystem::directory_iterator di(root);
				while (di != boost::filesystem::directory_iterator()) {
					auto s = load(di->path().leaf().string());
					if (s && isExpired(s)) {
						destroySession(s->id, Ice::Current());
					}
					di++;
				}
			}

			bool isExpired(SessionPtr s)
			{
				return (s->lastUsed + s->duration < time(NULL));
			}

			template<typename R, typename ER>
			R sysassert(R rtn, ER ertn)
			{
				if (rtn == ertn) {
					fprintf(stderr, "%s\n", strerror(errno));
					throw SessionError(strerror(errno));
				}
				return rtn;
			}

			Ice::CommunicatorPtr ic;
 			const boost::filesystem::path root;
			const Ice::Int duration;
	};
}
NAMEDFACTORY("IceSpider-FileSessions", IceSpider::FileSessions, IceSpider::PluginFactory);

