#include <gtkmm.h>
#include <iostream>
#include <deque>
#include <iostream>
#include <glibmm/threads.h>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/lexical_cast.hpp>
#include "telnet.h"
#ifdef POSIX
#include <termios.h>
#endif

using boost::asio::ip::tcp;
using namespace std;


class ScannerGUI;//forward declaration
class Worker{
	private:
	ScannerGUI* mainThread;
	Glib::Threads::Mutex mutex;
	deque<string> tx;
	deque<string> rx;
	void _processFiles();
	void addTextToTextArea(string text);
	void showProgress(string text, double progress);
	
	public:
	Worker(ScannerGUI* mainThread);
	void serverCommunicationThread();
	Glib::Dispatcher updateTextArea;
	Glib::Dispatcher updateProgress;
	string textAreaMessage;
	string progressMessage;
	double progressFraction;
	void processFiles();
	string folderToProcess;
	bool stop;
};


class ScannerGUI : public Gtk::Window
{

public:
  ScannerGUI();
  virtual ~ScannerGUI();
  Glib::Threads::Mutex mainMutex;
  void updateTextAreaMain();
  void updateProgressMain(); 
  Glib::Dispatcher doScan;

protected:
  //Signal handlers:
  void on_button_clicked();
  Glib::ustring x;
  Worker *worker;

  //Member widgets:
  Gtk::Box m_box1;
  Gtk::Button m_button1;
  Gtk::TextView m_TextView;  
  Gtk::ScrolledWindow m_ScrolledWindow;
  Glib::RefPtr<Gtk::TextBuffer> m_refTextBuffer1;
  Glib::Threads::Thread * m_thread; 
  
  Gtk::Button m_button2;
  Gtk::ProgressBar m_progress;
  void addTextToTextArea(string text);
  void updateProgress(string text, double progress);
  Glib::Dispatcher sig_message;

  

private:
  string folderToProcess;
};

void ScannerGUI::updateTextAreaMain(){
	Glib::Threads::Mutex::Lock lock (mainMutex); 
	m_refTextBuffer1->begin_user_action();	
	m_refTextBuffer1->insert_interactive_at_cursor(worker->textAreaMessage);
	m_refTextBuffer1->end_user_action();	
}

void ScannerGUI::updateProgressMain(){
	Glib::Threads::Mutex::Lock lock (mainMutex); 
	m_progress.set_text(worker->progressMessage);
	m_progress.set_fraction(worker->progressFraction);
}

void Worker::addTextToTextArea(string text){
	Glib::Threads::Mutex::Lock lock (mutex); 
	this->textAreaMessage = text;
	updateTextArea();	
}

void Worker::showProgress(string text, double progress){
	Glib::Threads::Mutex::Lock lock (mutex); 
	this->progressMessage = text;
	this->progressFraction = progress;
	updateProgress();
}

void Worker::processFiles(){
  Glib::Threads::Thread::create( sigc::mem_fun(*this,&Worker::_processFiles)); 	
}

void Worker::_processFiles(){
      using namespace boost::filesystem;
      rx.clear();
      path current_dir(folderToProcess); //
	  vector<string> filesToScan;
	  for (recursive_directory_iterator iter(current_dir), end;	iter != end; ++iter){
		if( boost::filesystem::is_regular_file( iter->path() ))
			filesToScan.push_back(iter->path().c_str());
		}
	  
		for(size_t i = 0; i < filesToScan.size(); i++) {
		  string s = filesToScan[i];
		  double progress = i;
		  progress /= filesToScan.size();
		  showProgress(s, progress);
		  tx.push_back("scanfile \""+s+"\"\n");
		  while( rx.size() == 0 )
			Glib::usleep(10000);
		  string str = rx.front();
		  rx.pop_front();
		  //std::cout << "RX: "<< str << endl;
			
		}	  
		showProgress("Done",1);
}


Worker::Worker(ScannerGUI* mainThread){
	this->mainThread = mainThread;
	this->stop = false;
}

void Worker::serverCommunicationThread() 
{ 
		// on Unix POXIS based systems, turn off line buffering of input, so cin.get() returns after every keypress
		// On other systems, you'll need to look for an equivalent
		#ifdef POSIX
			termios stored_settings;
			tcgetattr(0, &stored_settings);
			termios new_settings = stored_settings;
			new_settings.c_lflag &= (~ICANON);
			new_settings.c_lflag &= (~ISIG); // don't automatically handle control-C
			tcsetattr(0, TCSANOW, &new_settings);
		#endif
		try{
			boost::asio::io_service io_service;
			// resolve the host name and port number to an iterator that can be used to connect to the server
			tcp::resolver resolver(io_service);
			
			addTextToTextArea("Connecting to localhost port 65001...\n");
			tcp::resolver::query query("localhost", "65001");
			tcp::resolver::iterator iterator = resolver.resolve(query);
			// define an instance of the main class of this program
			telnet_client c(io_service, iterator);
			// run the IO service as a separate thread, so the main thread can block on standard input
			boost::thread t(boost::bind(&boost::asio::io_service::run, &io_service));	
			long counter = 0;
			while (1)
			{
				string str;
				counter++;
				bool dataRx = false;
				
				// Read from server
				c.mtx_.lock();
				while(c.readque.size() != 0){
					str += c.readque.front();
					c.readque.pop_front();
					dataRx = true;
				}
				c.mtx_.unlock();
				
				// Write responce to our waiting thread
				if( dataRx ){
					Glib::Threads::Mutex::Lock lock (mutex); 
					rx.push_back(str);
				}
				
				// Update text buffer
				if( dataRx ){
					addTextToTextArea(str);
				}
				

				//Write to server
				if( tx.size() != 0 ){
					{
						Glib::Threads::Mutex::Lock lock (mutex); 
						str = tx.front();
						tx.pop_front();
					}
					//cout << "tx: " << str << endl;
					addTextToTextArea(str);
					c.mtx_.lock();			
					for(size_t i = 0; i < str.length(); i++){
						c.write(str[i]);
					}
					c.mtx_.unlock();
				}
				
				Glib::usleep(10000);
				str = "";
				if(stop){
					Glib::Threads::Mutex::Lock lock (mutex);

						c.close();
						t.join();
						return;

				}

			}
			c.close(); // close the telnet client connection
			t.join(); // wait for the IO service thread to close			
		}catch(exception& e){
				cerr << "Exception: " << e.what() << "\n";
		}
		#ifdef POSIX // restore default buffering of standard input
			tcsetattr(0, TCSANOW, &stored_settings);
		#endif		
}


int main (int argc, char *argv[])
{
  Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(argc, argv, "org.gtkmm.example");
   if(!Glib::thread_supported()) Glib::thread_init();

  ScannerGUI scannerGui;

  //Shows the window and returns when it is closed.
  return app->run(scannerGui);
}

ScannerGUI::ScannerGUI():
 	m_button1("Select folder to scan"),   // creates a new button with label "Hello World".
	m_button2("hello"),
	m_box1(Gtk::ORIENTATION_VERTICAL)
  {
	//if(!Glib::thread_supported()) Glib::thread_init();
  // Sets the border width of the window.
  set_border_width(20);
  set_title("Scanner (c) Nicolai Tufar, 2013");
  set_position(Gtk::WindowPosition(Gtk::WIN_POS_CENTER));
  set_default_size(600, 400);
  
  
  
  

  // When the button receives the "clicked" signal, it will call the
  // on_button_clicked() method defined below.
  m_button1.signal_clicked().connect(sigc::mem_fun(*this,
              &ScannerGUI::on_button_clicked));

  m_box1.set_spacing(10);
  add(m_box1);
  m_box1.pack_start(m_button1, Gtk::PACK_SHRINK);
  m_button1.show();
  
  m_ScrolledWindow.add(m_TextView);
  m_refTextBuffer1 = Gtk::TextBuffer::create();
  //m_refTextBuffer1->set_text("This is the text from TextBuffer #1.");
  m_TextView.set_buffer(m_refTextBuffer1);
  m_TextView.show();
  m_ScrolledWindow.set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
  
  m_box1.pack_start(m_ScrolledWindow);
  m_ScrolledWindow.show();
  
  //m_box1.pack_start(m_button2);
  //m_button2.show();
  m_box1.pack_start(m_progress, Gtk::PACK_SHRINK);
  //m_progress.set_default_size(570, 50);
  m_progress.set_text("");
  m_progress.set_show_text(true);
  //m_progress.set_fraction(0.33);

  m_progress.show();
  m_box1.show();

  worker = new Worker(this);
  worker->updateTextArea.connect(sigc::mem_fun(*this, &ScannerGUI::updateTextAreaMain));
  worker->updateProgress.connect(sigc::mem_fun(*this, &ScannerGUI::updateProgressMain));
  this->doScan.connect(sigc::mem_fun(*worker, &Worker::processFiles));

  m_thread = Glib::Threads::Thread::create( sigc::mem_fun(*worker,&Worker::serverCommunicationThread)); 


}

ScannerGUI::~ScannerGUI()
{
	{
		Glib::Threads::Mutex::Lock lock (mainMutex);
		worker->stop = true;
	}
	if(m_thread)
		m_thread->join();
}

void ScannerGUI::on_button_clicked()
{
 
  Gtk::FileChooserDialog dialog("Please choose a folder",
          Gtk::FILE_CHOOSER_ACTION_SELECT_FOLDER);
  dialog.set_transient_for(*this);
  dialog.add_button(Gtk::Stock::CANCEL, Gtk::RESPONSE_CANCEL);
  dialog.add_button("Select", Gtk::RESPONSE_OK);
    
  int result = dialog.run();

 switch(result)
  {
    case(Gtk::RESPONSE_OK):
    {
	  dialog.hide();
      //std::cout << "Scanning: " << dialog.get_filename() << std::endl;
      folderToProcess = dialog.get_filename();
      worker->folderToProcess = folderToProcess;
      this->doScan();
      
      break;
    }
    case(Gtk::RESPONSE_CANCEL):
    {
      std::cout << "Cancel clicked." << std::endl;
      break;
    }
    default:
    {
      // Dialog closed
      break;
    }
  }
}
