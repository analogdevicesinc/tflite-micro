$project_folder = "music-genre-classification"
$env = "python_env"
$tflite_model_float32 = "float32_genre_ID.tflite"
$tflite_model_int8 = "int8_genre_ID.tflite"
$tflm_models = "TFLM_models"
$tflm_model1a = "float32_genre_ID_model_data.h"
$tflm_model1b = "float32_genre_ID_model_data.cc"
$tflm_model2a = "int8_genre_ID_model_data.h"
$tflm_model2b = "int8_genre_ID_model_data.cc"
$final_int8_model_path = "..\..\..\examples\genre_identification\common\model\int8_genre_ID\"
$final_float_model_path = "..\..\..\examples\genre_identification\common\model\float_genre_ID\"

function Remove-AllWithProgress {
    param (
        [Parameter(ValueFromPipeline=$true,Position=0)]
        $Path, 
        $Steps = 100
    )
    Remove-Item -Path $Path -Recurse -Force -Verbose 4>&1 | 
        ForEach-Object -begin {$c=0} -Process {
            if (-not (($c ++) % $steps)) {Write-progress -Activity "Deleting local temp files.. " -Status "$c files done" }
        }
}

if((Test-Path -Path $tflm_models\$tflm_model1a) -and (Test-Path -Path $tflm_models\$tflm_model1b) -and (Test-Path -Path $tflm_models\$tflm_model2a) -and (Test-Path -Path $tflm_models\$tflm_model2b)){
	Write-Host "TFLM models have been generated successfully"
}
else
{
	if(!(Test-Path -Path $project_folder)){
		git clone https://github.com/cetinsamet/music-genre-classification.git
	}
	else{
		Write-Host "Project directory already present, moving to next step"
	}
	
	cd scripts


	if(!(Test-Path -Path $env)){
		python -m pip install virtualenv
		python -m venv $env
		python_env\Scripts\activate
		pip install -r requirements.txt	
	}
	
	if(!(Test-Path -Path $tflite_model_int8)){
		python_env\Scripts\activate
		python convert_model.py
	}
	
	if(!(Test-Path -Path $tflm_models)){
		python generate_cc_arrays.py TFLM_models $tflite_model_float32
		python generate_cc_arrays.py TFLM_models $tflite_model_int8

	}
	
	if(Test-Path -Path ..\TFLM_models){
		Remove-Item -Path ..\TFLM_models -Recurse 
	}
	mv TFLM_models ../
	
	del $tflite_model_float32
	del $tflite_model_int8
	Write-Host "Reclaiming space... "
	Remove-AllWithProgress ($env )
	# Remove-Item -Path $env -Recurse
	cd ..
	Remove-Item -Path $project_folder -Recurse -Force
	if((Test-Path -Path $tflm_models\$tflm_model1a) -and (Test-Path -Path $tflm_models\$tflm_model1b) -and (Test-Path -Path $tflm_models\$tflm_model2a) -and (Test-Path -Path $tflm_models\$tflm_model2b)){
		Write-Host "TFLM models have been generated successfully"
	}
}


cp $tflm_models\$tflm_model2a $final_int8_model_path\$tflm_model2a
cp $tflm_models\$tflm_model2b $final_int8_model_path\$tflm_model2b
cp $tflm_models\$tflm_model1a $final_float_model_path\$tflm_model1a
cp $tflm_models\$tflm_model1b $final_float_model_path\$tflm_model1b

if((Test-Path -Path $final_float_model_path\$tflm_model1a) -and (Test-Path -Path $final_float_model_path\$tflm_model1b) -and (Test-Path -Path $final_int8_model_path\$tflm_model2a) -and (Test-Path -Path $final_int8_model_path\$tflm_model2b)){
	Write-Host "TFLM models have been copied into respective projects successfully, check the following paths to find the models:"
	Write-Host $final_int8_model_path
	Write-Host $final_float_model_path
}