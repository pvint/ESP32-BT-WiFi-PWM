const char INDEX_HTML[] PROGMEM = R"=====(
<html><head><title>VintLabs LED Controller</title></head><body>
<style>
.slidecontainer {
  width: 100%;
}

.slider {
  -webkit-appearance: none;
  width: 90%;
  margin: auto;
  height: 15px;
  border-radius: 5px;
  background: #d3d3d3;
  outline: none;
  opacity: 0.7;
  -webkit-transition: .2s;
  transition: opacity .2s;
}

.slider:hover {
  opacity: 1;
}

.slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  appearance: none;
  width: 25px;
  height: 25px;
  border-radius: 50%;
  background: #4CAF50;
  cursor: pointer;
}

.slider::-moz-range-thumb {
  width: 25px;
  height: 25px;
  border-radius: 50%;
  background: #4CAF50;
  cursor: pointer;
}
</style>
</head>
<body>

<h2>LED Controller</h1>

<div class="slidecontainer">
  Channel: <select id="ch">
  <option value="0">0</option>
  <option value="1">1</option>
  <option value="2">2</option>
  <option value="3">3</option>
  <option value="4">4</option>
  <option value="5">5</option>
  <option value="6">6</option>
  <option value="7">7</option>
  </select>
  <input type="range" min="0" max="4095" value="0" class="slider" id="led0val">
  <p>Value: <span id="led0text"></span></p>
</div>

<script>
var slider = document.getElementById("led0val");
var output = document.getElementById("led0text");
output.innerHTML = slider.value;

slider.onchange = function() {
	var val = this.value;
 	output.innerHTML = val;
	var ch = document.getElementById("ch").selectedIndex;

	var Http = new XMLHttpRequest();
	var url = "?ch=" + ch + "&dc=" + val;
	Http.open("GET", url);
	Http.send();
	
}
</script>

</body>
</html>
)=====";
