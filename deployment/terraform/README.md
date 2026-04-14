# Island Backend - Terraform Example Configuration

## Local Development

```hcl
# terraform.tfvars
aws_region      = "us-east-1"
environment     = "development"
vpc_id          = "vpc-12345678"
public_subnets  = ["subnet-12345678", "subnet-87654321"]
private_subnets = ["subnet-11111111", "subnet-22222222"]
docker_image_uri = "ghcr.io/island/backend:latest"
ecs_task_cpu     = 512
ecs_task_memory  = 1024
ecs_desired_count = 1
log_level        = "DEBUG"
jwt_secret       = "your-secure-random-32-char-string-here-123456"
```

## Production

```hcl
# terraform.tfvars
aws_region      = "us-east-1"
environment     = "production"
vpc_id          = "vpc-prod123456"
public_subnets  = ["subnet-prod1", "subnet-prod2", "subnet-prod3"]
private_subnets = ["subnet-private1", "subnet-private2", "subnet-private3"]
docker_image_uri = "123456789.dkr.ecr.us-east-1.amazonaws.com/island-backend:v1.0.0"
ecs_task_cpu     = 1024
ecs_task_memory  = 2048
ecs_desired_count = 3
log_level        = "INFO"
jwt_secret       = "$(openssl rand -base64 32)"
sns_topic_arn    = "arn:aws:sns:us-east-1:123456789:island-alerts"
```

## Deploy

```bash
# Initialize Terraform (first time only)
terraform init

# Plan deployment
terraform plan -var-file=terraform.tfvars -out=tfplan

# Apply
terraform apply tfplan

# Outputs
terraform output alb_dns_name
terraform output backend_url
```
