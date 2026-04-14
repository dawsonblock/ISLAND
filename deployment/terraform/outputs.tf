output "alb_dns_name" {
  description = "DNS name of the load balancer"
  value       = aws_lb.island_backend.dns_name
}

output "alb_arn" {
  description = "ARN of the load balancer"
  value       = aws_lb.island_backend.arn
}

output "ecs_cluster_name" {
  description = "Name of the ECS cluster"
  value       = aws_ecs_cluster.island.name
}

output "ecs_service_name" {
  description = "Name of the ECS service"
  value       = aws_ecs_service.island_backend.name
}

output "cloudwatch_log_group" {
  description = "CloudWatch log group name"
  value       = aws_cloudwatch_log_group.island_backend.name
}

output "backend_url" {
  description = "Backend service URL"
  value       = "http://${aws_lb.island_backend.dns_name}"
}
