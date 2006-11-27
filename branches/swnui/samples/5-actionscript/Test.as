class Test {

	static function run() {
		fscommand("save_file","file.txt:hello world !");
		_root.createTextField("tf",0,0,0,200,50);
		_root.tf.text = "File file.txt created !";
	}

	static var init = run();

}

