const modelViewer = document.querySelectorAll("model-viewer");
// To debug in console
// let material = modelViewer.model.materials;

for (const iteration of modelViewer) {
	iteration.addEventListener('load', () => {
	const material = iteration.model.materials;
	// material[material.length-1].pbrMetallicRoughness.setBaseColorFactor([224/255, 49/255, 29/255]); //SparkFun Red: #E0311D
	material[material.length-2].pbrMetallicRoughness.setBaseColorFactor([224/255, 0, 0]); // #E00000
	material[material.length-2].pbrMetallicRoughness.setMetallicFactor(.8);
	material[material.length-2].pbrMetallicRoughness.setRoughnessFactor(.7);
	});
};