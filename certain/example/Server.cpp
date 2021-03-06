#include "Certain.h"
#include "CertainUserImpl.h"
#include "DBImpl.h"
#include "PLogImpl.h"
#include "UserWorker.h"

static volatile uint8_t g_iStopFlag;

void SetStopFlag(int iSig)
{
	g_iStopFlag = 1;
}

int main(int argc, char **argv)
{
	signal(SIGINT, SetStopFlag);

	if (argc < 3)
	{
		printf("%s -c conf_path [options see certain.conf]\n", argv[0]);
		return -1;
	}

	clsCertainUserImpl oImpl;

    Certain::clsConfigure oConf(argc, argv);
    string strLID = to_string(oConf.GetLocalServerID());
    oConf.SetLogPath(oConf.GetCertainPath() + "/log_" + strLID);

    leveldb::DB* plog = NULL;
    string plogname = oConf.GetCertainPath() + "/plog_" + strLID;
    {
        leveldb::Status s;

        leveldb::Options opts;
        opts.create_if_missing = true;
        assert(leveldb::DB::Open(opts, plogname, &plog).ok());
    }

	clsKVEngine oKVEngineForPLog(plog);
    oKVEngineForPLog.SetNoThing();
	Certain::clsPLogImpl oPLogImpl(&oKVEngineForPLog);

    leveldb::DB* db = NULL;
    string dbname = oConf.GetCertainPath() + "/db_" + strLID;
    {
        leveldb::Status s;

        leveldb::Options opts;
        opts.create_if_missing = true;
        assert(leveldb::DB::Open(opts, dbname, &db).ok());
    }

	clsKVEngine oKVEngineForDB(db);
    oKVEngineForDB.SetUseMap();
	Certain::clsDBImpl oDBImpl(&oKVEngineForDB);

	Certain::clsCertainWrapper *poWrapper = NULL;

	poWrapper = Certain::clsCertainWrapper::GetInstance();
	assert(poWrapper != NULL);

	int iRet = poWrapper->Init(&oImpl, &oPLogImpl, &oDBImpl, &oConf);
	AssertEqual(iRet, 0);

    Certain::clsUserWorker::Init(10);
    for (uint32_t i = 0; i < 10; ++i)
    {
        Certain::clsUserWorker *poWorker = new Certain::clsUserWorker(i);
        poWorker->Start();
    }

	poWrapper->Start();

	while (1)
	{
		sleep(1);

		if (g_iStopFlag)
		{
            exit(-1);
		}
	}

	poWrapper->Destroy();

	return 0;
}
