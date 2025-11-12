use score_lifecycle_systemd::lifecycle::launch_manager::{LaunchManagerSD, AliveAPI, ControlAPI};

// mere usage example, this would run in an async loop
fn main() -> Result<(), Box<dyn std::error::Error>> {
    let service_name = "NetworkManager.service";

    let lm = LaunchManagerSD::default();
    
    lm.activate_run_target(service_name)?;
    lm.report_health_status(service_name)?;

    Ok(())
}
