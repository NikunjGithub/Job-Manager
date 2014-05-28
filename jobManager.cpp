#include <iostream>
#include <queue>
#include <boost/thread/thread.hpp>
#include <boost/asio.hpp>
#include <boost/tuple/tuple.hpp> 
#include <boost/tuple/tuple_io.hpp> 
#include <boost/function.hpp> 

///JOB Queue hold all jobs required to be executed
template<typename Job>
class JobQueue
{
  private:

    std::queue<Job> _queue;
    mutable boost::mutex _mutex;
    boost::condition_variable _conditionVariable;

  public:
    void push(Job const& job)
    {
      boost::mutex::scoped_lock lock(_mutex);
      _queue.push(job);
      lock.unlock();
      _conditionVariable.notify_one();
    }

    bool empty() const
    {
      boost::mutex::scoped_lock lock(_mutex);
      return _queue.empty();
    }

    bool tryPop(Job& poppedValue)
    {
      boost::mutex::scoped_lock lock(_mutex);
      if(_queue.empty())
      {
        return false;
      }

      poppedValue = _queue.front();
      _queue.pop();
      return true;
    }

    void waitAndPop(Job& poppedValue)
    {
      boost::mutex::scoped_lock lock(_mutex);
      while(_queue.empty())
      {
        _conditionVariable.wait(lock);
      }

      poppedValue = _queue.front();
      _queue.pop();
    }

};

///Thread pool for posting jobs to io service
class ThreadPool
{
  public :
    ThreadPool( int noOfThreads = 1) ;
    ~ThreadPool() ;

    template< class func >
      void post( func f ) ;

    boost::asio::io_service &getIoService() ;

  private :
    boost::asio::io_service _ioService;
    boost::asio::io_service::work _work ;
    boost::thread_group _threads;
};

  inline ThreadPool::ThreadPool( int noOfThreads )
: _work( _ioService )
{
  for(int i = 0; i < noOfThreads ; ++i) // 4
    _threads.create_thread(boost::bind(&boost::asio::io_service::run, &_ioService));
}

inline ThreadPool::~ThreadPool()
{
  _ioService.stop() ;
  _threads.join_all() ;
}

inline boost::asio::io_service &ThreadPool::getIoService()
{
  return _ioService ;
}

  template< class func >
void ThreadPool::post( func f )
{
  _ioService.post( f ) ;
}


template<typename T>
class Manager;

///Worker doing some work.
template<typename T>
class Worker{

    T _data;
    int _taskList;
    boost::mutex _mutex;
    Manager<T>* _hndl;

  public:

    Worker(T data, int task, Manager<T>* hndle):
    _data(data),
    _taskList(task),
    _hndl(hndle)
    {
    }

    bool job()
    {
      boost::mutex::scoped_lock lock(_mutex);
      std::cout<<"...Men at work..."<<++_data<<std::endl;
      --_taskList;
      if(taskDone())
       _hndl->end();
    } 

    bool taskDone()
    {
      std::cout<<"Tasks  "<<_taskList<<std::endl<<std::endl;
      if(_taskList == 0)
      {
        std::cout<<"Tasks done "<<std::endl;
        return true;
      }
      else false;
    }

};

///Job handler waits for new jobs and
///execute them as when a new job is received using Thread Pool.
//Once all jobs are done hndler exits.
template<typename T>
class Manager{

 public:

   typedef boost::function< bool (Worker<T>*)> Func;

   Manager(int threadCount):
   _threadCount(threadCount),
   _isWorkCompleted(false)
   {
     _pool = new ThreadPool(_threadCount);

     boost::thread jobRunner(&Manager::execute, this);
   }

   void add(Func f, Worker<T>* instance)
   {
     Job job(instance, f);
     _jobQueue.push(job);
   }

   void end()
   {
     boost::mutex::scoped_lock lock(_mutex);
     _isWorkCompleted = true;
     //send a dummy job
     add( NULL, NULL);
   }

   void workComplete()
   {
     std::cout<<"Job well done."<<std::endl;
   }

   bool isWorkDone()
   {
     boost::mutex::scoped_lock lock(_mutex);
     if(_isWorkCompleted)
       return true;
     return false;
   }

   void execute()
   {
      Job job;

     while(!isWorkDone())
     {
       _jobQueue.waitAndPop(job);

        Func f  = boost::get<1>(job);
        Worker<T>* ptr = boost::get<0>(job);
      
        if(f)
        {
          _pool->post(boost::bind(f, ptr));
        }
        else
          break;
     }

     std::cout<<"Complete"<<std::endl;
   }


 private:

  ThreadPool *_pool;
  int _threadCount;
  typedef boost::tuple<Worker<T>*, Func > Job;
  JobQueue<Job> _jobQueue;
  bool _isWorkCompleted;
  boost::mutex _mutex;
};

typedef boost::function< bool (Worker<int>*)> IntFunc;
typedef boost::function< bool (Worker<char>*)> CharFunc;


int main()
{
  boost::asio::io_service ioService;

  Manager<int> jobHndl(2);
  Worker<int> wrk1(0,4, &jobHndl);

  IntFunc f= &Worker<int>::job;

  jobHndl.add(f, &wrk1);
  jobHndl.add(f, &wrk1);
  jobHndl.add(f, &wrk1);
  jobHndl.add(f, &wrk1);

  Manager<char> jobHndl2(2);
  Worker<char> wrk2(0,'a', &jobHndl2);

  CharFunc f2= &Worker<char>::job;

  jobHndl2.add(f2, &wrk2);
  jobHndl2.add(f2, &wrk2);
  jobHndl2.add(f2, &wrk2);
  jobHndl2.add(f2, &wrk2);

  ioService.run();
  while(1){}
  return 0;
}

