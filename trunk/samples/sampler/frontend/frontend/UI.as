package frontend
{
	import swhx.Api;
	import mx.controls.TextArea;
	import mx.controls.List;
	import mx.controls.Button;
	import mx.controls.Alert;
	import mx.core.Application;
	import mx.core.UIComponent;
	import flash.utils.setTimeout;
	import mx.managers.PopUpManager;
	import mx.managers.PopUpManagerChildList;
	import BusyWindow;
	import mx.containers.TitleWindow;
			
	public class UI
	{		
		public var ui: Object;
		public var titles: Array;
		public var busyPopUp: BusyWindow;
	
		public function Sampler(): void {
			trace("Hello World, I am a Sampler instance");						
		}
		
		public function onUIReady(ui: Object): void {			
			trace("onUIReady");
			this.ui = ui;
			this.titles = new Array();			
			swhx.Api.init(this);
			swhx.Api.call("sampler.onUIReady");			
		}
		
		public function print(msg: String): void {
			var ta: TextArea = ui.txtConsole as TextArea;
			ta.text += msg + "\n";
			ta.validateNow();
			ta.verticalScrollPosition = ta.maxVerticalScrollPosition;			
		}
		
		public function notify(msg: String): void {
			Alert.show(msg, "Screenweaver HX Sampler"); 
		}
		
		public function list(title: Object): void {			
			titles.push(title);
			print("adding title: "+title.label);
			var l: List = ui.listSamples as List;
			l.dataProvider = titles;
			if (titles.length) {
				l.selectedIndex = 0;
				onListChange();
			}
		}
		
		public function onListChange(): void {
			var item: Object;
			var l: List = ui.listSamples as List;
			if ((item = l.selectedItem)) {
				var d: TextArea = ui.txtDescription as TextArea;
				d.htmlText = item.descr;
				d.verticalScrollPosition = 0;	
			}
		}
		
		public function run(index: int): void {						
			swhx.Api.call('sampler.run',index);			
		}
		
		public function allowRun(allow: Boolean): void {
			(ui.btnRun as Button).enabled = allow;
		}
		
		public function busy(): void {
			allowRun(false);
			busyPopUp = PopUpManager.createPopUp(ui as Application, BusyWindow, true) as BusyWindow;
			PopUpManager.centerPopUp(busyPopUp);
			busyPopUp.progress.setProgress(10,100);
		}
		
		public function busyUpdate(msg: String): void {
			busyPopUp.txtMessage.htmlText = msg;			
		}
		
		public function done(): void {
			busyPopUp.visible = false;
			PopUpManager.removePopUp(busyPopUp);
			allowRun(true);			
		}				
	}
}