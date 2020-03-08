#!/bin/bash

ROOTDIR=""
MOUNTDIR=""
ENV_VAR=""
DAEMON_PATH=""
GKFS_HOSTFILE_PATH=""
LISTEN=""
USE_MPI=false
USE_SRUN=false
NODES=""
NODENUM=1
MPI_HOSTS_ARGS=""
PRETEND=false

daemon_start() {
  ORTERUN=$(command -v orterun)
  # Setup
  local MPI_ENV_ARGS="-x PSM2_MULTI_EP=1 "
  if [[ -n ${ENV_VAR} ]]; then
    # split by comma and put into array
    IFS=',' read -r -a ENV_ARRAY <<< "${ENV_VAR}"
    for ELEMENT in "${ENV_ARRAY[@]}"; do
      if [[ ${USE_MPI} == true ]]; then
        MPI_ENV_ARGS+="-x ${ELEMENT} "
      else
        # set env variables for this shell to pass to srun
        export ELEMENT
      fi
    done
  fi
  # orterun --np 2 --map-by node --hostfile /home/vef/hostfile --enable-recovery -x PSM2_MULTI_EP=1 /home/vef/gekkofs/build/src/daemon/gkfs_daemon -r /tmp/rootdir -m /tmp/mountdir -H ~/vef_m2/gkfs_hostfile
  local GKFS_CMD="${DAEMON_PATH} -r ${ROOTDIR} -m ${MOUNTDIR} -H ${GKFS_HOSTFILE_PATH}"
  if [[ ${USE_MPI} == true ]]; then
    local START_CMD="${ORTERUN} -n ${NODENUM} --map-by node ${MPI_HOSTS_ARGS} ${MPI_ENV_ARGS} ${GKFS_CMD}"
  else
    local START_CMD="srun ${GKFS_CMD}"
  fi
  # Execution
  echo "[RUNNING]: ${START_CMD}"
  if [[ ${PRETEND} == true ]]; then
    echo "Just kidding."
    return
  fi
  # shellcheck disable=SC2086
  nohup ${START_CMD} >> /tmp/gkfs_launcher.log 2>&1 &
}

daemon_status() {
  echo "status"
}

daemon_stop() {
  echo "stop"
}

usage_short() {
  echo "
usage: launcher.sh [-h] [-r] [-m] [-e] [-p] [-l] [--use_mpi] {COMMAND}
	"
}

help_msg() {

  usage_short
  echo "
This script starts and stops GekkoFS daemons on multiple nodes by using pdsh

positional arguments:
        command                   Command to interact with the script: {start, stop, restart, status}

optional arguments:
        -h, --help                shows this help message and exits
        -r, --rootdir             Path where GekkoFS data is placed
        -m, --mountdir            Path where GekkoFS' virtual mountpoint is accessible
        -e, --env_var             Comma-separated list of environment variables: 'key1=value2,key2=value2'
        -p, --gkfs_daemon_path    Path to gkfs_deamon executable
        -H, --gkfs_hostfile       Path to where gkfs_daemons register their RPC endpoints (default: ./gkfs_hosts.txt)
        -l, --listen              Hostname suffix to listen on a specific network device (e.g., ib0 or eth0)
        --use_mpi                 Use orterun to start the daemons (EXPERIMENTAL)
        --use_srun                 Use srun to start the daemons (EXPERIMENTAL)
        -n, --nodes               Comma-separated list of nodes or a host file on where the daemons are/should run
        --pretend                 Doesn't execute commands, only prints what would be executed
        "
}

POSITIONAL=()
while [[ $# -gt 0 ]]; do
  key="$1"

  case ${key} in
  -r | --rootdir)
    ROOTDIR="$2"
    shift # past argument
    shift # past value
    ;;
  -m | --mountdir)
    MOUNTDIR="$2"
    shift # past argument
    shift # past value
    ;;
  -e | --env_var)
    ENV_VAR="$2"
    shift # past argument
    shift # past value
    ;;
  -p | --gkfs_daemon_path)
    DAEMON_PATH="$2"
    shift # past argument
    shift # past value
    ;;
  -H | --gkfs_hostfile)
    GKFS_HOSTFILE_PATH="$2"
    shift # past argument
    shift # past value
    ;;
  -l | --listen)
    LISTEN="$2"
    shift # past argument
    shift # past value
    ;;
  --use_mpi)
    USE_MPI=true
    shift # past argument
    ;;
  --use_srun)
    USE_SRUN=true
    shift # past argument
    ;;
  -n | --nodes)
    NODES="$2"
    shift # past argument
    shift # past value
    ;;
  --pretend)
    PRETEND=true
    shift # past argument
    ;;
  -h | --help)
    help_msg
    exit
    #shift # past argument
    ;;
  *) # unknown option
    POSITIONAL+=("$1") # save it in an array for later
    shift              # past argument
    ;;
  esac
done
set -- "${POSITIONAL[@]}" # restore positional parameters

# positional arguments
if [[ -z ${1+x} ]]; then
  echo "Positional arguments missing."
  usage_short
  exit 1
fi
CMD="${1}"
if [[ ${CMD} != "start" && ${CMD} != "restart" && ${CMD} != "stop" && ${CMD} != "status" ]]; then
  echo "Positional argument not valid: ${CMD}"
  exit 1
fi
# optional arguments
if [[ ! -e ${DAEMON_PATH} ]]; then
  echo "Path to GKFS daemon does not exist: ${DAEMON_PATH}"
  exit 1
fi
if [[ ! -d $(dirname "${GKFS_HOSTFILE_PATH}") ]]; then
  echo "Directory for gkfs hostfile does not exist: ${GKFS_HOSTFILE_PATH}"
  exit 1
fi
if [[ -z ${NODES} ]]; then
  echo "[INFO]: Running on 1 node"
else
  # check if given string is path or commaseparated list
  if [[ -e ${NODES} ]]; then
    MPI_HOSTS_ARGS="--hostfile ${NODES}"
    NODES_TYPE="HOSTFILE"
    NODENUM=$(wc -l < "${NODES}")
    echo "[INFO]: Running on ${NODENUM} nodes"
  else
    MPI_HOSTS_ARGS="-H ${NODES}"
    NODENUM=$(( $(grep -o "," <<< "${NODES}" | wc -l) + 1))
    echo "[INFO]: Running on ${NODENUM} nodes"
  fi
fi

case ${CMD} in
  start)
    daemon_start
    ;;
  restart)
    daemon_stop
    daemon_start
    ;;
  status)
    daemon_status
    ;;
  stop)
    daemon_stop
    ;;
  *)
    echo "Positional argument not valid: ${CMD}"
    exit 1
    ;;
esac