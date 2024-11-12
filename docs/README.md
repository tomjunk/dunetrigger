# dunetrigger

Tools for emulating dunedaq trigger flow and analyzing trigger information in the DUNE experiment within the LArSoft framework.

## Apptainer container

Currently, the apptainer container is used to run the LArSoft trigger emulation software. In this [link](https://wiki.dunescience.org/wiki/SL7_to_Alma9_conversion#Running_SL7_inside_a_container), one can find the instructions to set up the apptainer environment, which is a containerized environment for running `dunesw` tools on SL7 instead of using AL9, still under development.

To sum up, the following command can be used to run the apptainer container:

```bash
/cvmfs/oasis.opensciencegrid.org/mis/apptainer/current/bin/apptainer shell --shell=/bin/bash -B /cvmfs,/exp,/nashome,/pnfs/dune,/opt,/run/user,/etc/hostname,/etc/hosts,/etc/krb5.conf --ipc --pid /cvmfs/singularity.opensciencegrid.org/fermilab/fnal-dev-sl7:latest
```

## Create and set up the development environment

The bash snippet below creates and sets up the development environment for the LArSoft trigger emulation software used in the DUNE experiment. To do so, one just needs to copy the code into an executable bash script and run it.

It is worth noting that the username must be adapted to the user running the script. Regarding the software version, it is set to `v09_91_04d00`, but it can be changed to any other future version available in the DUNE software environment. The same applies to the qualifiers, which are set to `e26:prof` in this case.

```bash
#This script sets up the development environment for the LArSoft trigger emulation software used in the DUNE experiment.
# It creates a working directory, sets up the required software packages, and clones the necessary repositories.
# Finally, it builds and installs the software.

VERSION=v09_91_04d00  # Main version of the software to be used
QUALS=e26:prof  # Qualifiers for the software packages
DIRECTORY=trigger_sim_dev  # Name of the directory where the software will be installed
export WORKDIR=/exp/dune/app/users/$USER/  # Path to the working directory

# Create the working directory if it doesn't exist
if [ ! -d "$WORKDIR" ]; then
  mkdir -p "$WORKDIR"
fi

# Set up the DUNE software environment
source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh
export UPS_OVERRIDE="-H Linux64bit+3.10-2.17"
setup dunesw -v ${VERSION} -q ${QUALS}

cd ${WORKDIR}
mkdir -p ${DIRECTORY}
cd ${DIRECTORY}

# Create a new development area using mrb
mrb newDev -v ${VERSION} -q ${QUALS}
source ${WORKDIR}/${DIRECTORY}/localProducts*/setup

# Clone the dunetrigger repository and DUNE-DAQ/triggeralgs submodule (https://github.com/DUNE-DAQ/triggeralgs/tree/production/v4)
mrb g https://github.com/wesketchum/dunetrigger.git
cd srcs/dunetrigger
git checkout schhibra/daq_triggeralgs
git submodule init
git submodule update

# Make the required changes in triggeralgs directory
bash create_cmakelists.sh
bash replace_triggeralgs.sh
cd ${WORKDIR}/${DIRECTORY}

# Clone the duneprototypes repository
# Temporary until the RDTimestamp fix is merged into develop
mrb g https://github.com/wesketchum/duneprototypes.git
git checkout feature/wketchum_FixRawDigitRDTimestamps

cd ${MRB_BUILDDIR}

mrbslp

# Set up the environment for building and installing the software
# In gpvm machines, use -j3 to avoid memory issues, since dune gpvms have 4 cores only. Build virtual machines are not available with the Apptainer container. In another virtual machine we use half of the cores available to build the software on stronger machines.
mrbsetenv

# Get the number of CPU cores
num_cores=$(nproc)
# Calculate half the number of cores. scale=0 rounds the result to the integer part.
half_cores=$(echo "scale=0; $num_cores / 2" | bc)
# Get the hostname
hostname=$(hostname)
# Check if the hostname contains 'dunegpvm'
if [[ "$hostname" == *dunegpvm* ]]; then
    half_cores=3
fi
echo "Number of cores to use: ${half_cores}"

mrb i -j${half_cores}
mrbslp
```

## Source the local products after creating the development area

Once the development area is created, the environment can be set up by running the following commands within a bash script:

```bash
# This script sets up the environment for the DUNE software and runs the necessary commands to source the local products after creating the development area.

VERSION=v09_91_04d00  # Version of the software to be used
QUALS=e26:prof  # Qualifiers for the software packages

# Source the setup script for the DUNE software
source /cvmfs/dune.opensciencegrid.org/products/dune/setup_dune.sh

# Set the UPS_OVERRIDE environment variable to specify the desired platform
export UPS_OVERRIDE="-H Linux64bit+3.10-2.17"

# Setup the specific version of the DUNE software
setup dunesw ${VERSION} -q ${QUALS}

# Source the setup script for the local products associated to the development area
source localProducts_*/setup

# Set up the MRB source local products
mrbslp
```

As mentioned in the previous [section](#create-and-set-up-the-development-environment), both the software version and qualifiers can be changed to any other available in the DUNE software environment.

## Generate trigger information from real raw data

Generating trigger information from real data is essential to validate the correct performance of trigger algorithms already available in the DAQ framework. The goal is to compare the trigger information generated in LArSoft with the trigger information obtained from the DAQ framework and stored in the *Trigger Records*.

The following subsections are devoted to generate trigger information (TPs, TAs and TCs) from raw waveforms from data taking using the LArSoft trigger emulation software.

### How to decode a raw trigger data file

The following command shows how to decode a raw trigger data file using the LArSoft trigger emulation software.

```bash
lar -c run_pdhd_tpc_decoder.fcl -n ${N_EVENTS} -s ${RAW_FILE_PATH} -o ${DECODED_FILE_PATH} -T ${DECODED_HIST_PATH}
```

Here is a brief explanation of the different flags used in the command:

- `-n ${N_EVENTS}`: specifies the number of events to be processed.
- `-s ${RAW_FILE_PATH}`: specifies the path to the raw trigger data file, an HDF5 file with trigger objects. E.g., `np04hd_raw_run026305_0033_dataflow0_datawriter_0_20240520T133910.hdf5`.
- `-o ${DECODED_FILE_PATH}`: specifies the path for the output decoded art ROOT file.
- `-T ${DECODED_HIST_PATH}`: specifies the path for the output histogram ROOT file.

The configuration file `run_pdhd_tpc_decoder.fcl` can be found in the `/srcs/dunetrigger/example` directory of the dunetrigger repository.

In case one desires to use a raw data file for tests, one can find an example raw data file in the `/pnfs/dune/persistent/users/hamza/trigger_sim_testing` directory.

For the example raw data file, the command would be:

```bash
lar -c srcs/dunetrigger/example/run_pdhd_tpc_decoder.fcl -n -1 -s /pnfs/dune/persistent/users/hamza/trigger_sim_testing/np04hd_raw_run026305_0033_dataflow0_datawriter_0_20240520T133910.hdf5
```

The `-1` flag indicates that all events in the file will be processed. The decoded file `np04hd_raw_run026305_0033_dataflow0_datawriter_0_20240520T133910_decode.root` is created.

### How to run the LArSoft trigger emulation

The following command shows how to run the LArSoft trigger emulation software for the decoded file, which contains the raw waveforms needed for TPG finding.

```bash
lar -c srcs/dunetrigger/example/run_tpalg_taalg_tcalg_online_ADCSimpleWindow.fcl -n ${N_EVENTS} -s ${DECODED_FILE_PATH} -o ${TRIGGER_FILE_PATH} -T ${TRIGGER_HIST_PATH}
```

In this command, `${N_EVENTS}` represents the number of events to be processed. `${DECODED_FILE_PATH}` corresponds to the path of the decoded art ROOT file. `${TRIGGER_FILE_PATH}` denotes the path for the output art ROOT data file. Lastly, `${TRIGGER_HIST_PATH}` indicates the path for the output histogram ROOT file.

`run_tpalg_taalg_tcalg_online_ADCSimpleWindow.fcl` runs SimpleThreshold algorithm (in `/srcs/dunetrigger/TPAlgTools/`) for TP generation, ADCSimpleWindow algorithm (in `srcs/dunetrigger/dunetrigger/triggeralgs/src`) for TA and TC generation:

```c++
algconfig_ta:
{
	window_length: 60000
	adc_threshold: 20000000
}

algconfig_tc:
{
}

physics:
{

 producers:
 {
     tpmakerTPC:
     {
         module_type: TriggerPrimitiveMakerTPC
         rawdigit_tag: "tpcrawdecoder:daq"
         tpalg: {
	      threshold_tpg_plane0: -1
	           threshold_tpg_plane1: -1
		        threshold_tpg_plane2: 60
             tool_type: TPAlgTPCSimpleThreshold
             verbosity: 0
         }
         verbosity: 1
     }

     tamakerTPC:
     {
         module_type: TriggerActivityMakerOnlineTPC
         tp_tag: "tpmakerTPC"
	  algorithm: "TriggerActivityMakerADCSimpleWindowPlugin"
         algconfig_plane0: @local::algconfig_ta 
         algconfig_plane1: @local::algconfig_ta
         algconfig_plane2: @local::algconfig_ta
         algconfig_plane3: @local::algconfig_ta
         verbosity: 1
     }

     tcmakerTPC:
     {
         module_type: TriggerCandidateMakerOnlineTPC
         ta_tag: "tamakerTPC"
         algorithm: "TriggerCandidateMakerADCSimpleWindowPlugin"
         algconfig: @local::algconfig_tc
         verbosity: 1
     }
 }
}
 ```
Similarly, the `run_tpalg_taalg_tcalg_online_ChannelAdjacency.fcl` file uses ChannelAdjacency algorithm for TA and TC generation.
### How to compare the trigger data

To compare offline and online trigger information, use the `run_triggerTPCInfoComparator.fcl` configuration file. This analysis helps identify any differences between the trigger data generated by the trigger emulation software and the trigger information saved in the *Trigger Records*. It provides histograms that highlight the similarities and discrepancies between offline and online trigger information. This analysis validates the correctness and consistency of the trigger algorithms in LArSoft, ensuring accurate event selection for simulated events.

To execute the configuration file `run_triggerTPCInfoComparator.fcl`, one can utilize the following art instruction:

```shell
lar -c srcs/dunetrigger/example/run_triggerTPCInfoComparator.fcl -n ${N_EVENTS} -s ${TRIGGER_FILE_PATH} -o ${TRIGGER_COMPARATOR_FILE_PATH} -T ${TRIGGER_COMPARATOR_HIST_PATH}
```

In this instruction, `${N_EVENTS}` specifies the number of events to process. `${TRIGGER_FILE_PATH}` denotes the location of the trigger data file, while `${TRIGGER_COMPARATOR_FILE_PATH}` points to the output art ROOT data file location. `${TRIGGER_COMPARATOR_HIST_PATH}` indicates the location for the output histogram ROOT file.

## Generate trigger information from simulated data with LArSoft

Using the art ROOT file generated at the `detsim` level, one can generate trigger information by means of the LArSoft trigger emulation software. It allows for the emulation of the trigger flow and the analysis of the trigger information in the DUNE experiment within the LArSoft framework. This is of utmost importance when analyzing signals from simulated data to tune up algorithm parameters, or even improving and validating the trigger algorithms.

### LArSoft trigger emulation from simulated data

To perform the LArSoft trigger emulation, just execute the `run_tpalg_taalg_tcalg_online_ADCSimpleWindow.fcl` file as in the previous [section](#how-to-run-the-larsoft-trigger-emulation) for the decoded art ROOT file. Keep in mind that the input file, `${DETSIM_FILE_PATH}`,  is the art ROOT file generated at the `detsim` level.

```bash
lar -c srcs/dunetrigger/example/run_tpalg_taalg_tcalg_online_ADCSimpleWindow.fcl -n ${N_EVENTS} -s ${DETSIM_FILE_PATH} -o ${TRIGGER_FILE_PATH} -T ${TRIGGER_HIST_PATH}
```

### How to analyze the trigger data

To analyze trigger data from simulated detector signals, use the `run_offlineTriggerTPCInfoDisplay.fcl` configuration file. This analysis provides insights into the accuracy and consistency of trigger information generated by the trigger emulation software. It is suitable not only to test new algorithms, but also to fine-tune algorithm parameters, allowing for proper event selections. It includes histograms highlighting trigger primitives (TPs), trigger activities (TAs), and trigger candidates (TCs). By examining these outputs, one can understand the performance and behaviour of trigger algorithms and ensure the reliability of the trigger emulation system.

To execute the analysis, one can use the following command:

```bash
lar -c srcs/dunetrigger/example/run_offlineTriggerTPCInfoDisplay.fcl -n ${N_EVENTS} -s ${TRIGGER_FILE_PATH} -o ${ANALYSIS_FILE_PATH} -T ${ANALYSIS_HIST_PATH}
```

In this command, `${N_EVENTS}` is the number of events to be processed, `${TRIGGER_FILE_PATH}` is the path to the trigger data file, `${ANALYSIS_FILE_PATH}` is the path for the output art ROOT data file, and `${ANALYSIS_HIST_PATH}` is the path for the output histogram ROOT file.
