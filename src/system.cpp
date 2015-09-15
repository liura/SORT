/*
   FileName:      system.cpp

   Created Time:  2011-08-04 12:41:44

   Auther:        Cao Jiayin

   Email:         soraytrace@hotmail.com

   Location:      China, Shanghai

   Description:   SORT is short for Simple Open-source Ray Tracing. Anyone could checkout the source code from
                'sourceforge', https://soraytrace.svn.sourceforge.net/svnroot/soraytrace. And anyone is free to
                modify or publish the source code. It's cross platform. You could compile the source code in 
                linux and windows , g++ or visual studio 2008 is required.
*/

// include the header file
#include "system.h"
#include "managers/texmanager.h"
#include "managers/logmanager.h"
#include "managers/meshmanager.h"
#include "managers/matmanager.h"
#include "managers/memmanager.h"
#include "utility/timer.h"
#include "texture/rendertarget.h"
#include "geometry/intersection.h"
#include "utility/path.h"
#include "utility/creator.h"
#include "sampler/sampler.h"
#include "multithread/parallel.h"
#include <ImfHeader.h>
#include "utility/strhelper.h"
#include "camera/camera.h"
#include "integrator/integrator.h"
#include "sampler/stratified.h"
#include <time.h>
#include "output/sortoutput.h"

// constructor
System::System()
{
	_preInit();
}
// destructor
System::~System()
{
	_postUninit();
}

// pre-initialize
void System::_preInit()
{
	// initialize log manager
	LogManager::CreateInstance();
	// initialize texture manager
	TexManager::CreateInstance();
	// initialize mesh manager
	MeshManager::CreateInstance();
	// initialize material manager
	MatManager::CreateInstance();
	// initialize memory manager
	MemManager::CreateInstance();
	// initialize the timer
	Timer::CreateInstance();

	// setup default value
	m_rt = 0;
	m_camera = 0;
	m_uRenderingTime = 0;
	m_uPreProcessingTime = 0;
	m_uPreProgress = 0xffffffff;
	m_thread_num = 1;
}

// post-uninit
void System::_postUninit()
{
	// relase the memory
	m_Scene.Release();

	// delete the data
	SAFE_DELETE( m_rt );
	SAFE_DELETE( m_camera );
	SAFE_DELETE( m_pSampler );
	SAFE_DELETE_ARRAY( m_taskDone );

	// release managers
	Creator::DeleteSingleton();
	MatManager::DeleteSingleton();
	TexManager::DeleteSingleton();
	MeshManager::DeleteSingleton();
	MemManager::DeleteSingleton();
	Timer::DeleteSingleton();
	LogManager::DeleteSingleton();
}

// render the image
void System::Render()
{
	// pre-process before rendering
	PreProcess();

	// set timer before rendering
	Timer::GetSingleton().StartTimer();

	// push rendering task
	_pushRenderTask();
	// execute rendering tasks
	_executeRenderingTasks();
	
	// stop timer
	m_uRenderingTime = Timer::GetSingleton().StopTimer();
}

// output render target
void System::OutputRT()
{
	//m_rt->Output( m_OutputFileName );
	//image_output.PostProcess();
}

// load the scene
bool System::LoadScene( const string& filename )
{
	string str = GetFullPath(filename);
	return m_Scene.LoadScene( str );
}

// pre-process before rendering
void System::PreProcess()
{
	// set timer before pre-processing
	Timer::GetSingleton().StartTimer();

	if( m_rt == 0 )
	{
		LOG_WARNING<<"There is no render target in the system, can't render anything."<<ENDL;
		return;
	}
	if( m_camera == 0 )
	{
		LOG_WARNING<<"There is no camera attached in the system , can't render anything."<<ENDL;
		return;
	}

	// preprocess scene
	m_Scene.PreProcess();

	// stop timer
	Timer::GetSingleton().StopTimer();
	m_uPreProcessingTime = Timer::GetSingleton().GetElapsedTime();

	// output some information
	_outputPreprocess();
}

// output preprocessing information
void System::_outputPreprocess()
{
	unsigned	core_num = NumSystemCores();
	cout<<"------------------------------------------------------------------------------"<<endl;
	cout<<" SORT is short for Simple Open-source Ray Tracing."<<endl;
	cout<<"   Multi-thread is enabled"<<"("<<core_num<<" core"<<((core_num>1)?"s are":" is")<<" detected.)"<<endl;
	cout<<"   "<<m_iSamplePerPixel<<" sample"<<((m_iSamplePerPixel>1)?"s are":" is")<<" used per pixel."<<endl;
	cout<<"   Scene file : "<<m_Scene.GetFileName()<<endl;
	cout<<"   Time spent on preprocessing :"<<m_uPreProcessingTime<<" ms."<<endl;
	cout<<"------------------------------------------------------------------------------"<<endl;
}

// get elapsed time
unsigned System::GetRenderingTime() const
{
	return m_uRenderingTime;
}

// output progress
void System::_outputProgress()
{
	// get the number of tasks done
	unsigned taskDone = 0;
	for( unsigned i = 0; i < m_totalTask; ++i )
		taskDone += m_taskDone[i];

	// output progress
	unsigned progress = (unsigned)( (float)(taskDone) / (float)m_totalTask * 100 );
	cout<< progress<<"\rProgress: ";
}

// output log information
void System::OutputLog() const
{
	// output scene information first
	m_Scene.OutputLog();

	// output time information
	LOG_HEADER( "Rendering Information" );
	LOG<<"Time spent on pre-processing  : "<<m_uPreProcessingTime<<ENDL;
	LOG<<"Time spent on rendering       : "<<m_uRenderingTime<<ENDL;
}

// uninitialize 3rd party library
void System::_uninit3rdParty()
{
	Imf::staticUninitialize();
}

// uninitialize
void System::Uninit()
{
	_uninit3rdParty();
}

// push rendering task
void System::_pushRenderTask()
{
	// Push render task into the queue
	unsigned tilesize = 64;
	unsigned taskid = 0;
	
	// get the number of total task
	m_totalTask = 0;
	for( unsigned i = 0 ; i < m_rt->GetHeight() ; i += tilesize )
		for( unsigned j = 0 ; j < m_rt->GetWidth() ; j += tilesize )
			++m_totalTask;
	m_taskDone = new bool[m_totalTask];
	memset( m_taskDone , 0 , m_totalTask * sizeof(bool) );

	image_output.SetImageSize( m_rt->GetWidth() , m_rt->GetHeight() );
	blender_output.SetImageSize( m_rt->GetWidth() , m_rt->GetHeight() );
	m_outputs.push_back( &image_output );
	m_outputs.push_back( &blender_output );

	RenderTask rt(m_Scene,m_pSampler,m_camera,m_outputs,m_taskDone,m_iSamplePerPixel);

	int tile_num_x = ceil(m_rt->GetWidth() / (float)tilesize);
	int tile_num_y = ceil(m_rt->GetHeight() / (float)tilesize);

	// start tile from center instead of top-left corner
	int current_x = tile_num_x / 2;
	int current_y = tile_num_y / 2;
	int cur_dir = 0;
	int cur_len = 0;
	int cur_dir_len = 1;
	int dir_x[4] = { 0, -1, 0, 1 };	// down , left , up , right
	int dir_y[4] = { -1, 0, 1, 0 };	// down , left , up , right
	while (true)
	{
		// only process node inside the image region
		if (current_x >= 0 && current_x < tile_num_x && current_y >= 0 && current_y < tile_num_y)
		{
			rt.taskId = taskid++;
			rt.ori_x = current_x * tilesize;
			rt.ori_y = current_y * tilesize;
			rt.width = (tilesize < (m_rt->GetWidth() - rt.ori_x)) ? tilesize : (m_rt->GetWidth() - rt.ori_x);
			rt.height = (tilesize < (m_rt->GetHeight() - rt.ori_y)) ? tilesize : (m_rt->GetHeight() - rt.ori_y);

			// create new pixel samples
			rt.pixelSamples = new PixelSample[m_iSamplePerPixel];

			// push the render task
			PushRenderTask(rt);
		}

		// turn to the next direction
		if (cur_len >= cur_dir_len)
		{
			cur_dir = (cur_dir + 1) % 4;
			cur_len = 0;
			cur_dir_len += 1 - cur_dir % 2;
		}

		// get to the next tile
		current_x += dir_x[cur_dir];
		current_y += dir_y[cur_dir];
		++cur_len;

		// stop further processing if there is no need
		if( (current_x < 0 || current_x >= tile_num_x) && (current_y < 0 || current_y >= tile_num_y) )
			break;
	}
}

// do ray tracing in a multithread enviroment
void System::_executeRenderingTasks()
{
	vector<SORTOutput*>::const_iterator it = m_outputs.begin();
	while( it != m_outputs.end() )
	{
		(*it)->PreProcess();
		++it;
	}

	InitCriticalSections();

	// will be parameterized later
	const int THREAD_NUM = m_thread_num;

	// pre allocate memory for the specific thread
	for( int i = 0 ; i < THREAD_NUM ; ++i )
		MemManager::GetSingleton().PreMalloc( 1024 * 1024 * 16 , i );
	ThreadUnit** threadUnits = new ThreadUnit*[THREAD_NUM];
	for( int i = 0 ; i < THREAD_NUM ; ++i )
	{
		// spawn new threads
		threadUnits[i] = SpawnNewRenderThread(i);
		
		// setup basic data
		threadUnits[i]->m_pIntegrator = _allocateIntegrator();
		threadUnits[i]->m_pIntegrator->PreProcess();
	}

	for( int i = 0 ; i < THREAD_NUM ; ++i ){
		// start new thread
		threadUnits[i]->BeginThread();
	}

	// wait for all the threads to be finished
	while( true )
	{
		bool allfinished = true;
		for( int i = 0 ; i < THREAD_NUM ; ++i )
		{
			//threadUnits[i]->WaitForFinish();
			if( !threadUnits[i]->IsFinished() )
				allfinished = false;
		}
		
		// Output progress
		_outputProgress();

		if( allfinished )
			break;
	}
	for( int i = 0 ; i < THREAD_NUM ; ++i )
		delete threadUnits[i];
	delete[] threadUnits;

	DestroyCriticalSections();

	cout<<endl;

	it = m_outputs.begin();
	while( it != m_outputs.end() )
	{
		(*it)->PostProcess();
		++it;
	}
}

// allocate integrator
Integrator*	System::_allocateIntegrator()
{
	Integrator* integrator = CREATE_TYPE( m_integratorType , Integrator );
		
	if( integrator == 0 )
	{
		LOG_WARNING<<"No integrator with name of "<<m_integratorType<<"."<<ENDL;
		return 0;
	}

	vector<Property>::iterator it = m_integratorProperty.begin();
	while( it != m_integratorProperty.end() )
	{
		integrator->SetProperty(it->_name,it->_property);
		++it;
	}

	return integrator;
}

// setup system from file
bool System::Setup( const char* str )
{
	// load the xml file
	string full_name = GetFullPath(str);
	TiXmlDocument doc( full_name.c_str() );
	doc.LoadFile();
	
	// if there is error , return false
	if( doc.Error() )
	{
		LOG_ERROR<<doc.ErrorDesc()<<CRASH;
		return false;
	}
	
	// get the root of xml
	TiXmlNode*	root = doc.RootElement();
	
	// try to load the scene , note: only the first node matters
	TiXmlElement* element = root->FirstChildElement( "Scene" );
	if( element )
	{
		const char* str_scene = element->Attribute( "value" );
		if( str_scene )
			LoadScene(str_scene);
		else
			return false;
	}else
		return false;
	
	// get the integrater
	element = root->FirstChildElement( "Integrator" );
	if( element )
	{
		m_integratorType = element->Attribute( "type" );

		// set the properties
		TiXmlElement* prop = element->FirstChildElement( "Property" );
		while( prop )
		{
			const char* prop_name = prop->Attribute( "name" );
			const char* prop_value = prop->Attribute( "value" );

			Property property;
			property._name = prop_name;
			property._property = prop_value;
			m_integratorProperty.push_back( property );

			prop = prop->NextSiblingElement( "Property" );
		}
	}else
		return false;
	
	// get the render target
	m_rt = new RenderTarget();
	element = root->FirstChildElement( "RenderTargetSize" );
	if( element )
	{
		const char* str_width = element->Attribute("w");
		const char* str_height = element->Attribute("h");
		
		if( str_width && str_height )
		{
			unsigned width = atoi( str_width );
			unsigned height = atoi( str_height );
		
			if( width < 1 ) width = 1;
			if( height < 1) height = 1;
		
			m_rt->SetSize( width , height );
		}
	}else
	{
		// use 1024x768 image as default
		m_rt->SetSize(1024, 768);
	}
	
	// get sampler
	element = root->FirstChildElement( "Sampler" );
	if( element )
	{
		const char* str_type = element->Attribute("type");
		const char* str_round = element->Attribute("round");
		
		unsigned round = atoi( str_round );
		if( round < 1 ) round = 1;
		if( round > 1024 ) round = 1024;
		
		// create sampler
		m_pSampler = CREATE_TYPE( str_type , Sampler );
		m_iSamplePerPixel = m_pSampler->RoundSize(round);
	}else{
		// user stratified sampler as default sampler
		m_pSampler = new StratifiedSampler();
		m_iSamplePerPixel = m_pSampler->RoundSize(16);
	}
	
	element = root->FirstChildElement("Camera");
	if( element )
	{
		const char* str_camera = element->Attribute("type");
	
		// create the camera
		m_camera = CREATE_TYPE(str_camera,Camera);
		
		if( !m_camera )
			return false;
		
		// set the properties
		TiXmlElement* prop = element->FirstChildElement( "Property" );
		while( prop )
		{
			const char* prop_name = prop->Attribute( "name" );
			const char* prop_value = prop->Attribute( "value" );
			if( prop_name != 0 && prop_value != 0 )
				m_camera->SetProperty( prop_name , prop_value );
			prop = prop->NextSiblingElement( "Property" );
		}
	}

	element = root->FirstChildElement("OutputFile");
	if( element )
		m_OutputFileName = element->Attribute("name");
	else
		m_OutputFileName = "default.bmp";	// make a default filename

	element = root->FirstChildElement("ThreadNum");
	if( element )
		m_thread_num = atoi(element->Attribute("name"));

	// setup render target
	m_camera->SetRenderTarget(m_rt);

	return true;
}
