##init_code_sharc_fx

To make the loader file of the target application boot-ready, it needs an initialization sequence which is defined in:  
`Eagle-TFLM\Utils\flashing-tools\init_code_sharc_fx`

Define the init file path in the application project:  
`Project properties > C/C++ Build > Settings > Tool Settings > CrossCore SHARC-FX Loader > Initialization`

Ensure the entire path to the `init_code_sharc_fx.dxe` file (generated after running the `init_code_sharc_fx CCES` application) is added. `init_code_sharc_fx.dxe` gets generated in Release or Debug folder after building `Eagle-TFLM\Utils\flashing-tools\init_code_sharc_fx` CCES porject.
