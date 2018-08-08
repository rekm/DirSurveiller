#!/usr/bin/env bash

# Inspired by OP of
# https://stackoverflow.com/questions/48540257/
# caching-virtual-environment-for-gitlab-ci

#ENV_NAME="qcumber"
#if [[ -z "${CI_PROJECT_DIR}" ]]; then
#  CI_PROJECT_DIR="."
#fi
#
#if [ ! -d "$CI_PROJECT_DIR/.conda_cache/$ENV_NAME" ]; then
#    echo "Environment $ENV_NAME does not exist. Creating it now!"
#    conda-env create -f environment/packages.yaml \
#    -p "$CI_PROJECT_DIR/.conda_cache/$ENV_NAME"
#else
#    echo "Updating $ENV_NAME with packages.yaml"
#    conda-env update -f environment/packages.yaml \
#    -p "$CI_PROJECT_DIR/.conda_cache/$ENV_NAME"
#fi
#
#echo "Activating environment: $CI_PROJECT_DIR/.conda_cache/$ENV_NAME"
#source activate $CI_PROJECT_DIR/.conda_cache/qcumber

#"$CI_PROJECT_DIR/.conda_cache/$ENV_NAME"

