# ========== Clone the libraries and install the IDF ==========

# Mostly taken from: https://github.com/espressif/esp-idf/blob/master/tools/docker/Dockerfile

FROM ubuntu:22.04 AS upstream

# switch to root
USER root
SHELL ["/bin/bash", "-c"]

ARG DEBIAN_FRONTEND=noninteractive

ARG FIRMWARE_VERSION=1.0.5

# Get required packages
RUN : \
  && apt-get update \
  && apt-get install -y \
    apt-utils \
    bison \
    bzip2 \
    ca-certificates \
    ccache \
    check \
    curl \
    flex \
    git \
    git-lfs \
    gperf \
    lcov \
    libbsd-dev \
    libffi-dev \
    libglib2.0-0 \
    libncurses-dev \
    libpixman-1-0 \
    libsdl2-2.0-0 \
    libslirp0 \
    libusb-1.0-0-dev \
    make \
    ninja-build \
    python3 \
    python3-venv \
    ruby \
    unzip \
    wget \
    xz-utils \
    zip \
  && apt-get autoremove -y \
  && rm -rf /var/lib/apt/lists/* \
  && update-alternatives --install /usr/bin/python python /usr/bin/python3 10 \
  && :

# Avoid the externally managed environment constraint
#RUN PYTHON_VER=$(ls /usr/lib | grep python3.) \
#    && echo "Python version: ${PYTHON_VER}" \
#    && rm /usr/lib/${PYTHON_VER}/EXTERNALLY-MANAGED

# To build the image for a branch or a tag of IDF, pass --build-arg IDF_CLONE_BRANCH_OR_TAG=name.
# To build the image with a specific commit ID of IDF, pass --build-arg IDF_CHECKOUT_REF=commit-id.
# It is possible to combine both, e.g.:
#   IDF_CLONE_BRANCH_OR_TAG=release/vX.Y
#   IDF_CHECKOUT_REF=<some commit on release/vX.Y branch>.
# Use IDF_CLONE_SHALLOW=1 to perform shallow clone (i.e. --depth=1 --shallow-submodules)
# Use IDF_CLONE_SHALLOW_DEPTH=X to define the depth if IDF_CLONE_SHALLOW is used (i.e. --depth=X)
# Use IDF_INSTALL_TARGETS to install tools only for selected chip targets (CSV)

# Use: https://github.com/espressif/esp-idf/releases/tag/v5.1.5
ARG IDF_CLONE_URL=https://github.com/espressif/esp-idf.git
ARG IDF_CLONE_BRANCH_OR_TAG=release/v5.1
ARG IDF_CHECKOUT_REF=454782b
ARG IDF_CLONE_SHALLOW=
ARG IDF_CLONE_SHALLOW_DEPTH=1
ARG IDF_INSTALL_TARGETS=esp32
ARG IDF_GITHUB_ASSETS=

ENV IDF_PATH=/opt/esp/idf
ENV IDF_TOOLS_PATH=/opt/esp
ENV IDF_GITHUB_ASSETS=${IDF_GITHUB_ASSETS}

# install build essential needed for linux target apps, which is a preview target so it is installed with "all" only
RUN if [ "$IDF_INSTALL_TARGETS" = "all" ]; then \
    apt-get update \
    && apt-get install -y build-essential \
    && apt-get autoremove -y \
    && rm -rf /var/lib/apt/lists/* ; \
  fi

RUN echo IDF_CHECKOUT_REF=$IDF_CHECKOUT_REF IDF_CLONE_BRANCH_OR_TAG=$IDF_CLONE_BRANCH_OR_TAG && \
    git clone --recursive \
      ${IDF_CLONE_SHALLOW:+--depth=${IDF_CLONE_SHALLOW_DEPTH} --shallow-submodules} \
      ${IDF_CLONE_BRANCH_OR_TAG:+-b $IDF_CLONE_BRANCH_OR_TAG} \
      $IDF_CLONE_URL $IDF_PATH && \
    git config --system --add safe.directory $IDF_PATH && \
    if [ -n "$IDF_CHECKOUT_REF" ]; then \
      cd $IDF_PATH && \
      if [ -n "$IDF_CLONE_SHALLOW" ]; then \
        git fetch origin --depth=${IDF_CLONE_SHALLOW_DEPTH} --recurse-submodules ${IDF_CHECKOUT_REF}; \
      fi && \
      git checkout $IDF_CHECKOUT_REF && \
      git submodule update --init --recursive; \
    fi

# Install all the required tools
RUN : \
  && update-ca-certificates --fresh \
  && $IDF_PATH/tools/idf_tools.py --non-interactive install required --targets=${IDF_INSTALL_TARGETS} \
  && $IDF_PATH/tools/idf_tools.py --non-interactive install cmake \
  && $IDF_PATH/tools/idf_tools.py --non-interactive install-python-env \
  && rm -rf $IDF_TOOLS_PATH/dist \
  && :

# Add get_idf as alias
RUN echo 'alias get_idf=". /opt/esp/idf/export.sh"' >> /root/.bashrc

# The constraint file has been downloaded and the right Python package versions installed. No need to check and
# download this at every invocation of the container.
ENV IDF_PYTHON_CHECK_CONSTRAINTS=no

# Ccache is installed, enable it by default
ENV IDF_CCACHE_ENABLE=1

# ===========================================================================================

# Copy RTK_mosaic-X5_Firmware and build deployment image
FROM upstream AS deployment

# Add the source files
ADD . .

# Update the version
RUN sed -i "s|Firmware v0.0.0|Firmware v$FIRMWARE_VERSION|g" RTK_mosaic-X5_Firmware/main/RTK_mosaic-X5_Firmware.c

# Build firmware
RUN cd ./RTK_mosaic-X5_Firmware \
    && set -e \
    && . $IDF_PATH/export.sh \
    && idf.py build

# ===========================================================================================

# Copy the build output. List the files
FROM deployment AS output
COPY --from=deployment ./RTK_mosaic-X5_Firmware/build/RTK_mosaic-X5_Firmware.* /
COPY --from=deployment ./RTK_mosaic-X5_Firmware/build/bootloader/bootloader.bin /
COPY --from=deployment ./RTK_mosaic-X5_Firmware/build/partition_table/partition-table.bin /
CMD echo $(ls /*.*)
