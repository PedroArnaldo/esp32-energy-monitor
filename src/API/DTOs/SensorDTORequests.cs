namespace API.DTOs;

public class SensorDTORequests
{
    public string Equipment { get; set; }
    public string Current { get; set; }
    public string Power { get; set; }
    public string Location { get; set; }
    public string Unit { get; set; }
    public DateTime Timestamp { get; set; }
}