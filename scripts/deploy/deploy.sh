#!/bin/bash
# Deployment script for Nilotic Blockchain
# Production deployment automation

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# Configuration
PROJECT_NAME="nilotic_blockchain"
DEPLOY_DIR="/opt/nilotic-blockchain"
SERVICE_NAME="nilotic-blockchain"
USER="nilotic"
GROUP="nilotic"

echo -e "${BLUE}🚀 Deploying Nilotic Blockchain${NC}"

# Function to check if running as root
check_root() {
    if [ "$EUID" -ne 0 ]; then
        echo -e "${RED}❌ This script must be run as root${NC}"
        exit 1
    fi
}

# Function to create system user
create_user() {
    echo -e "${YELLOW}👤 Creating system user...${NC}"
    
    if ! id "$USER" &>/dev/null; then
        useradd -r -s /bin/false -d "$DEPLOY_DIR" "$USER"
        echo -e "${GREEN}✅ Created user $USER${NC}"
    else
        echo -e "${YELLOW}⚠️  User $USER already exists${NC}"
    fi
}

# Function to create deployment directory
create_deploy_dir() {
    echo -e "${YELLOW}📁 Creating deployment directory...${NC}"
    
    mkdir -p "$DEPLOY_DIR"
    mkdir -p "$DEPLOY_DIR/logs"
    mkdir -p "$DEPLOY_DIR/data"
    mkdir -p "$DEPLOY_DIR/config"
    
    chown -R "$USER:$GROUP" "$DEPLOY_DIR"
    chmod -R 755 "$DEPLOY_DIR"
    
    echo -e "${GREEN}✅ Created deployment directory${NC}"
}

# Function to copy application files
copy_files() {
    echo -e "${YELLOW}📦 Copying application files...${NC}"
    
    # Copy executable
    if [ -f "build/$PROJECT_NAME" ]; then
        cp "build/$PROJECT_NAME" "$DEPLOY_DIR/"
        chmod +x "$DEPLOY_DIR/$PROJECT_NAME"
        echo -e "${GREEN}✅ Copied executable${NC}"
    else
        echo -e "${RED}❌ Executable not found. Please build first.${NC}"
        exit 1
    fi
    
    # Copy web wallet
    if [ -d "web" ]; then
        cp -r web "$DEPLOY_DIR/"
        echo -e "${GREEN}✅ Copied web wallet${NC}"
    fi
    
    # Copy configuration
    if [ -d "config" ]; then
        cp -r config/* "$DEPLOY_DIR/config/"
        echo -e "${GREEN}✅ Copied configuration${NC}"
    fi
    
    # Copy documentation
    if [ -d "docs" ]; then
        cp -r docs "$DEPLOY_DIR/"
        echo -e "${GREEN}✅ Copied documentation${NC}"
    fi
}

# Function to create systemd service
create_service() {
    echo -e "${YELLOW}🔧 Creating systemd service...${NC}"
    
    cat > "/etc/systemd/system/$SERVICE_NAME.service" << EOF
[Unit]
Description=Nilotic Blockchain
After=network.target

[Service]
Type=simple
User=$USER
Group=$GROUP
WorkingDirectory=$DEPLOY_DIR
ExecStart=$DEPLOY_DIR/$PROJECT_NAME --port 5500 --data-dir $DEPLOY_DIR/data
Restart=always
RestartSec=10
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target
EOF

    systemctl daemon-reload
    systemctl enable "$SERVICE_NAME"
    
    echo -e "${GREEN}✅ Created systemd service${NC}"
}

# Function to create nginx configuration
create_nginx_config() {
    echo -e "${YELLOW}🌐 Creating nginx configuration...${NC}"
    
    if command -v nginx &> /dev/null; then
        cat > "/etc/nginx/sites-available/$SERVICE_NAME" << EOF
server {
    listen 80;
    server_name localhost;

    # API proxy
    location /api/ {
        proxy_pass http://localhost:5500/;
        proxy_set_header Host \$host;
        proxy_set_header X-Real-IP \$remote_addr;
        proxy_set_header X-Forwarded-For \$proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto \$scheme;
    }

    # Web wallet
    location / {
        root $DEPLOY_DIR/web/wallet;
        index index.html;
        try_files \$uri \$uri/ /index.html;
    }
}
EOF

        ln -sf "/etc/nginx/sites-available/$SERVICE_NAME" "/etc/nginx/sites-enabled/"
        nginx -t && systemctl reload nginx
        
        echo -e "${GREEN}✅ Created nginx configuration${NC}"
    else
        echo -e "${YELLOW}⚠️  Nginx not found, skipping web server configuration${NC}"
    fi
}

# Function to create firewall rules
create_firewall_rules() {
    echo -e "${YELLOW}🔥 Creating firewall rules...${NC}"
    
    if command -v ufw &> /dev/null; then
        ufw allow 5500/tcp
        ufw allow 80/tcp
        echo -e "${GREEN}✅ Created firewall rules${NC}"
    elif command -v firewall-cmd &> /dev/null; then
        firewall-cmd --permanent --add-port=5500/tcp
        firewall-cmd --permanent --add-port=80/tcp
        firewall-cmd --reload
        echo -e "${GREEN}✅ Created firewall rules${NC}"
    else
        echo -e "${YELLOW}⚠️  No firewall manager found${NC}"
    fi
}

# Function to start the service
start_service() {
    echo -e "${YELLOW}🚀 Starting service...${NC}"
    
    systemctl start "$SERVICE_NAME"
    
    # Wait for service to start
    sleep 5
    
    if systemctl is-active --quiet "$SERVICE_NAME"; then
        echo -e "${GREEN}✅ Service started successfully${NC}"
    else
        echo -e "${RED}❌ Failed to start service${NC}"
        systemctl status "$SERVICE_NAME"
        exit 1
    fi
}

# Function to test deployment
test_deployment() {
    echo -e "${YELLOW}🧪 Testing deployment...${NC}"
    
    # Test API
    if curl -s http://localhost:5500/ > /dev/null; then
        echo -e "${GREEN}✅ API is responding${NC}"
    else
        echo -e "${RED}❌ API is not responding${NC}"
        return 1
    fi
    
    # Test web wallet
    if [ -f "$DEPLOY_DIR/web/wallet/index.html" ]; then
        echo -e "${GREEN}✅ Web wallet files found${NC}"
    else
        echo -e "${RED}❌ Web wallet files missing${NC}"
        return 1
    fi
}

# Function to show deployment info
show_info() {
    echo -e "\n${BLUE}📋 Deployment Information${NC}"
    echo -e "${BLUE}========================${NC}"
    echo -e "Service Name: $SERVICE_NAME"
    echo -e "Deploy Directory: $DEPLOY_DIR"
    echo -e "User: $USER"
    echo -e "API URL: http://localhost:5500"
    echo -e "Web Wallet: http://localhost"
    echo -e ""
    echo -e "${YELLOW}📋 Useful Commands:${NC}"
    echo -e "  systemctl status $SERVICE_NAME"
    echo -e "  systemctl restart $SERVICE_NAME"
    echo -e "  systemctl stop $SERVICE_NAME"
    echo -e "  journalctl -u $SERVICE_NAME -f"
    echo -e "  curl http://localhost:5500/"
}

# Main deployment function
main() {
    echo -e "${BLUE}🚀 Starting deployment...${NC}"
    
    check_root
    create_user
    create_deploy_dir
    copy_files
    create_service
    create_nginx_config
    create_firewall_rules
    start_service
    test_deployment
    show_info
    
    echo -e "${GREEN}🎉 Deployment completed successfully!${NC}"
}

# Run main function
main "$@" 