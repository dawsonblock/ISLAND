terraform {
  backend "s3" {
    bucket         = "island-terraform-state"
    key            = "prod/terraform.tfstate"
    region         = "us-east-1"
    encrypt        = true
    dynamodb_table = "island-terraform-lock"
  }
}
