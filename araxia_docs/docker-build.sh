#!/bin/bash
# TrinityCore Docker Build Helper Script
# This script automates the Docker build and setup process

set -e

# Default values
IMAGE_REGISTRY=""
IMAGE_NAME="trinitycore"
IMAGE_TAG="latest"
PLATFORM="linux/amd64"  # Default to amd64
MULTI_ARCH=false
PUSH=false

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if command exists
command_exists() {
    command -v "$1" >/dev/null 2>&1
}

# Setup QEMU for cross-platform builds
setup_qemu() {
    local host_arch=$(uname -m)
    local needs_qemu=false
    
    # Check if we're building for a different architecture
    if [[ "$PLATFORM" == *"arm64"* ]] && [ "$host_arch" != "aarch64" ]; then
        needs_qemu=true
    elif [[ "$PLATFORM" == *"amd64"* ]] && [ "$host_arch" != "x86_64" ]; then
        needs_qemu=true
    fi
    
    if [ "$needs_qemu" = true ]; then
        print_info "Cross-platform build detected, setting up QEMU emulation..."
        
        # Check if QEMU is already set up
        if docker run --rm --privileged multiarch/qemu-user-static --version >/dev/null 2>&1; then
            print_info "Installing QEMU binfmt handlers..."
            docker run --rm --privileged multiarch/qemu-user-static --reset -p yes >/dev/null 2>&1
            print_success "QEMU emulation configured!"
        else
            print_warning "Could not set up QEMU automatically."
            print_warning "You may need to run: docker run --rm --privileged multiarch/qemu-user-static --reset -p yes"
        fi
    fi
}

# Check prerequisites
check_prerequisites() {
    print_info "Checking prerequisites..."
    
    if ! command_exists docker; then
        print_error "Docker is not installed. Please install Docker first."
        exit 1
    fi
    
    if ! command_exists docker-compose && ! docker compose version >/dev/null 2>&1; then
        print_error "Docker Compose is not installed. Please install Docker Compose first."
        exit 1
    fi
    
    # Check for buildx if multi-arch build is requested
    if [ "$MULTI_ARCH" = true ]; then
        if ! docker buildx version >/dev/null 2>&1; then
            print_error "Docker buildx is required for multi-architecture builds."
            print_error "Please install Docker buildx or use a single platform build."
            exit 1
        fi
        
        # Create or use existing buildx builder
        if ! docker buildx inspect multiarch-builder >/dev/null 2>&1; then
            print_info "Creating buildx builder for multi-architecture builds..."
            docker buildx create --name multiarch-builder --use
        else
            print_info "Using existing buildx builder..."
            docker buildx use multiarch-builder
        fi
    fi
    
    # Setup QEMU for cross-platform builds
    setup_qemu
    
    print_success "All prerequisites met!"
}

# Create necessary directories
create_directories() {
    print_info "Creating necessary directories..."
    
    mkdir -p etc logs data sql-import
    
    print_success "Directories created!"
}

# Setup environment file
setup_env() {
    if [ ! -f .env ]; then
        print_info "Creating .env file from template..."
        cp .env.example .env
        print_warning "Please edit .env file and set secure passwords!"
    else
        print_info ".env file already exists, skipping..."
    fi
}

# Build Docker images
build_images() {
    print_info "Building Docker images (this may take 10-30 minutes)..."
    print_info "Build will use all available CPU cores. If you have low RAM, edit Dockerfile and reduce -j value."
    
    # Construct full image names
    if [ -n "$IMAGE_REGISTRY" ]; then
        FULL_PREFIX="${IMAGE_REGISTRY}/${IMAGE_NAME}"
    else
        FULL_PREFIX="${IMAGE_NAME}"
    fi
    
    print_info "Image registry: ${IMAGE_REGISTRY:-<none>}"
    print_info "Image name: $IMAGE_NAME"
    print_info "Image tag: $IMAGE_TAG"
    print_info "Platform(s): $PLATFORM"
    
    if [ "$MULTI_ARCH" = true ]; then
        # Multi-architecture build using buildx
        print_info "Building multi-architecture image..."
        
        BUILD_CMD="docker buildx build"
        BUILD_CMD="$BUILD_CMD --platform $PLATFORM"
        BUILD_CMD="$BUILD_CMD -t ${FULL_PREFIX}:${IMAGE_TAG}"
        BUILD_CMD="$BUILD_CMD -f Dockerfile"
        
        if [ "$PUSH" = true ]; then
            BUILD_CMD="$BUILD_CMD --push"
            print_info "Images will be pushed to registry after build"
        else
            BUILD_CMD="$BUILD_CMD --load"
            print_warning "Multi-arch builds with --load only support single platform"
            print_warning "Use --push to build and push multi-platform images to registry"
        fi
        
        BUILD_CMD="$BUILD_CMD ."
        
        print_info "Running: $BUILD_CMD"
        eval $BUILD_CMD
        
    else
        # Single platform build using docker-compose
        # Check if we should use docker-compose or docker compose
        if command_exists docker-compose; then
            COMPOSE_CMD="docker-compose"
        else
            COMPOSE_CMD="docker compose"
        fi
        
        # Export variables for docker-compose
        export IMAGE_REGISTRY
        export IMAGE_NAME
        export IMAGE_TAG
        export DOCKER_DEFAULT_PLATFORM=$PLATFORM
        
        print_info "Building for platform: $PLATFORM"
        $COMPOSE_CMD build
    fi
    
    print_success "Docker images built successfully!"
    print_success "Image tagged as:"
    print_success "  - ${FULL_PREFIX}:${IMAGE_TAG}"
}

# Copy configuration files
setup_configs() {
    print_info "Checking for configuration files..."
    
    # Find worldserver.conf.dist
    WORLDSERVER_DIST=$(find . -name "worldserver.conf.dist" -o -name "authserver.conf.dist" | grep worldserver.conf.dist | head -n1)
    AUTHSERVER_DIST=$(find . -name "authserver.conf.dist" -o -name "bnetserver.conf.dist" | head -n1)
    
    if [ -n "$WORLDSERVER_DIST" ] && [ ! -f etc/worldserver.conf ]; then
        print_info "Copying worldserver.conf.dist to etc/worldserver.conf..."
        cp "$WORLDSERVER_DIST" etc/worldserver.conf
        print_warning "Please edit etc/worldserver.conf and configure database settings!"
    fi
    
    if [ -n "$AUTHSERVER_DIST" ] && [ ! -f etc/authserver.conf ]; then
        print_info "Copying authserver config to etc/authserver.conf..."
        cp "$AUTHSERVER_DIST" etc/authserver.conf
        print_warning "Please edit etc/authserver.conf and configure database settings!"
    fi
    
    if [ -z "$WORLDSERVER_DIST" ]; then
        print_warning "Could not find worldserver.conf.dist - you'll need to provide config files manually"
    fi
}

# Display next steps
show_next_steps() {
    echo ""
    print_success "============================================="
    print_success "TrinityCore Docker Build Complete!"
    print_success "=============================================="
    echo ""
    # Construct full image names
    if [ -n "$IMAGE_REGISTRY" ]; then
        FULL_PREFIX="${IMAGE_REGISTRY}/${IMAGE_NAME}"
    else
        FULL_PREFIX="${IMAGE_NAME}"
    fi
    
    print_info "Built image:"
    echo "  - ${FULL_PREFIX}:${IMAGE_TAG}"
    echo ""
    print_info "This single image is used for both authserver and worldserver."
    echo "The entrypoint is set in docker-compose.yml for each service."
    echo ""
    
    if [ -n "$IMAGE_REGISTRY" ] || [ "$IMAGE_TAG" != "latest" ]; then
        print_info "To push to registry:"
        echo "  docker push ${FULL_PREFIX}:${IMAGE_TAG}"
        echo ""
    fi
    
    print_info "Next steps:"
    echo ""
    echo "  1. Edit .env file and set secure passwords"
    echo "     Also set IMAGE_REGISTRY, IMAGE_NAME, and IMAGE_TAG if using custom images"
    echo ""
    echo "  2. Edit configuration files in etc/ directory:"
    echo "     - etc/worldserver.conf"
    echo "     - etc/authserver.conf"
    echo "     Set database host to 'mysql' instead of localhost"
    echo ""
    echo "  3. Import SQL files to database:"
    echo "     docker compose up -d mysql"
    echo "     # Wait 30 seconds for MySQL to start"
    echo "     docker compose exec mysql mysql -uroot -p<password> -e \"CREATE DATABASE auth;\""
    echo "     docker compose exec mysql mysql -uroot -p<password> -e \"CREATE DATABASE characters;\""
    echo "     docker compose exec mysql mysql -uroot -p<password> -e \"CREATE DATABASE world;\""
    echo "     docker compose exec -T mysql mysql -uroot -p<password> auth < sql/base/auth_database.sql"
    echo "     docker compose exec -T mysql mysql -uroot -p<password> characters < sql/base/characters_database.sql"
    echo ""
    echo "  4. Extract client data (maps, vmaps, mmaps)"
    echo "     See DOCKER_BUILD_README.md for extraction commands"
    echo ""
    echo "  5. Start services:"
    echo "     docker compose up -d"
    echo ""
    echo "  6. View logs:"
    echo ""
    print_info "For detailed instructions, see DOCKER_BUILD_README.md"
    echo ""
}

# Show usage
show_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -r, --registry REGISTRY  Container registry (e.g., ghcr.io/araxiaonline)"
    echo "  -n, --name NAME          Image name (default: trinitycore)"
    echo "  -t, --tag TAG            Image tag/version (default: latest)"
    echo "  -p, --platform PLATFORM  Target platform (default: linux/amd64)"
    echo "                           Examples: linux/amd64, linux/arm64, linux/amd64,linux/arm64"
    echo "  -m, --multi-arch         Enable multi-architecture build (requires buildx)"
    echo "  --push                   Push image to registry after build (for multi-arch)"
    echo "  -h, --help               Show this help message"
    echo ""
    echo "Examples:"
    echo "  # Build with default settings (amd64 only)"
    echo "  $0"
    echo ""
    echo "  # Build for ARM64"
    echo "  $0 --platform linux/arm64"
    echo ""
    echo "  # Build multi-architecture image (amd64 + arm64) and push to registry"
    echo "  $0 --registry ghcr.io/araxiaonline --tag 11.2.5.63834 \\"
    echo "     --platform linux/amd64,linux/arm64 --multi-arch --push"
    echo ""
    echo "  # Build for local testing (single platform)"
    echo "  $0 -r ghcr.io/myorg -n tc-server -t v1.0.0 -p linux/amd64"
    echo ""
}

# Parse command line arguments
parse_args() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -r|--registry)
                IMAGE_REGISTRY="$2"
                shift 2
                ;;
            -n|--name)
                IMAGE_NAME="$2"
                shift 2
                ;;
            -t|--tag)
                IMAGE_TAG="$2"
                shift 2
                ;;
            -p|--platform)
                PLATFORM="$2"
                shift 2
                ;;
            -m|--multi-arch)
                MULTI_ARCH=true
                shift
                ;;
            --push)
                PUSH=true
                shift
                ;;
            -h|--help)
                show_usage
                exit 0
                ;;
            *)
                print_error "Unknown option: $1"
                show_usage
                exit 1
                ;;
        esac
    done
    
    # Validate multi-arch settings
    if [ "$MULTI_ARCH" = true ] && [[ "$PLATFORM" == *","* ]]; then
        print_info "Multi-architecture build enabled for platforms: $PLATFORM"
        if [ "$PUSH" = false ]; then
            print_warning "Multi-arch builds without --push will only load the first platform"
            print_warning "Consider using --push to build all platforms and push to registry"
        fi
    fi
}

# Main execution
main() {
    parse_args "$@"
    
    echo ""
    print_info "=============================================="
    print_info "TrinityCore Docker Build Script"
    print_info "=============================================="
    echo ""
    
    check_prerequisites
    create_directories
    setup_env
    build_images
    setup_configs
    show_next_steps
}

# Run main function
main "$@"
