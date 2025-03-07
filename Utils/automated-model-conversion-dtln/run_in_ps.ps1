$project_folder = "dtln"
$env = "python_env"
$tflm_models = "TFLM_models"
$tflm_model1a = "model_int16_1_data.h"
$tflm_model1b = "model_int16_1_data.cc"
$tflm_model2a = "model_int16_2_data.h"
$tflm_model2b = "model_int16_2_data.cc"
$tflm_f_model1a = "model_float_1_data.h"
$tflm_f_model1b = "model_float_1_data.cc"
$tflm_f_model2a = "model_float_2_data.h"
$tflm_f_model2b = "model_float_2_data.cc"


$final_int16_model_path = "..\..\examples\denoiser_realtime\model_int16"
$final_int16_model_path2 = "..\..\examples\denoiser_fileio\src\model_int16"
$final_float_model_path = "..\..\examples\denoiser_realtime\model_fp"
$final_float_model_path2 = "..\..\examples\denoiser_fileio\src\model_fp"

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
		git clone https://github.com/breizhn/DTLN.git dtln
	}
	else{
		Write-Host "Project directory already present, moving to next step"
	}

	cp scripts\DTLN_model.py dtln\DTLN_model.py
	cp scripts\input1.npy dtln\input1.npy
	cp scripts\input2.npy dtln\input2.npy
	cp scripts\state1.npy dtln\state1.npy
	cp scripts\state2.npy dtln\state2.npy
	cp scripts\fix_error.py dtln\fix_error.py
	cd $project_folder
	
	py -3.10 -m pip install virtualenv
	py -3.10 -m venv $env
	python_env\Scripts\activate
	pip install tensorflow==2.15.0 soundfile wavinfo
	
	python fix_error.py
	python convert_weights_to_tf_lite.py -m pretrained_model/model.h5 -t model_float
	python convert_weights_to_tf_lite.py -m pretrained_model/model.h5 -t model_int16 -q true

	cp model_int16_1.tflite ..\scripts\
	cp model_int16_2.tflite ..\scripts\
	cp model_float_1.tflite ..\scripts\
	cp model_float_2.tflite ..\scripts\
	
	
	cd ..\scripts
	if(!(Test-Path -Path $tflm_models)){
		python generate_cc_arrays.py TFLM_models model_int16_2.tflite
		python generate_cc_arrays.py TFLM_models model_int16_1.tflite
		python generate_cc_arrays.py TFLM_models model_float_1.tflite
		python generate_cc_arrays.py TFLM_models model_float_2.tflite

	}
	if(Test-Path -Path ..\TFLM_models){
		Remove-Item -Path ..\TFLM_models -Recurse 
	}
	mv TFLM_models ../
	del model_int16_1.tflite
	del model_int16_2.tflite
	del model_float_1.tflite
	del model_float_2.tflite


	cd ..
	cd $project_folder
	Write-Host "Reclaiming space... "
	Remove-AllWithProgress ($env )
	cd ..
	Remove-Item -Path $project_folder -Recurse -Force
	if((Test-Path -Path $tflm_models\$tflm_model1a) -and (Test-Path -Path $tflm_models\$tflm_model1b) -and (Test-Path -Path $tflm_models\$tflm_model2a) -and (Test-Path -Path $tflm_models\$tflm_model2b)){
		Write-Host "TFLM models have been generated successfully"
	}
	
}
#Copy to denoiser_fileio
cp $tflm_models\$tflm_model2a $final_int16_model_path\$tflm_model2a
cp $tflm_models\$tflm_model2b $final_int16_model_path\$tflm_model2b
cp $tflm_models\$tflm_model1a $final_int16_model_path\$tflm_model1a
cp $tflm_models\$tflm_model1b $final_int16_model_path\$tflm_model1b

cp $tflm_models\$tflm_f_model1a $final_float_model_path\$tflm_f_model1a
cp $tflm_models\$tflm_f_model1b $final_float_model_path\$tflm_f_model1b
cp $tflm_models\$tflm_f_model2a $final_float_model_path\$tflm_f_model2a
cp $tflm_models\$tflm_f_model2b $final_float_model_path\$tflm_f_model2b

#Copy to denoiser_realtime
cp $tflm_models\$tflm_f_model1a $final_float_model_path2\$tflm_f_model1a
cp $tflm_models\$tflm_f_model1b $final_float_model_path2\$tflm_f_model1b
cp $tflm_models\$tflm_f_model2a $final_float_model_path2\$tflm_f_model2a
cp $tflm_models\$tflm_f_model2b $final_float_model_path2\$tflm_f_model2b

cp $tflm_models\$tflm_model2a $final_int16_model_path2\$tflm_model2a
cp $tflm_models\$tflm_model2b $final_int16_model_path2\$tflm_model2b
cp $tflm_models\$tflm_model1a $final_int16_model_path2\$tflm_model1a
cp $tflm_models\$tflm_model1b $final_int16_model_path2\$tflm_model1b

if((Test-Path -Path $final_int16_model_path\$tflm_model1a) -and (Test-Path -Path $final_int16_model_path\$tflm_model1b) -and (Test-Path -Path $final_int16_model_path\$tflm_model2a) -and (Test-Path -Path $final_int16_model_path\$tflm_model2b)){
	Write-Host "TFLM models have been copied into respective projects successfully, check the following paths to find the models:"
	Write-Host $final_int16_model_path
	Write-Host $final_float_model_path
}

if((Test-Path -Path $final_int16_model_path2\$tflm_model1a) -and (Test-Path -Path $final_int16_model_path2\$tflm_model1b) -and (Test-Path -Path $final_int16_model_path2\$tflm_model2a) -and (Test-Path -Path $final_int16_model_path2\$tflm_model2b)){
	Write-Host "TFLM models have been copied into respective projects successfully, check the following paths to find the models:"
	Write-Host $final_int16_model_path2
	Write-Host $final_float_model_path2
}