namespace API.Models
{
    public class Sensor
    {
        public int Id { get; set; }
        public string Equipment { get; set; }
        public string Current { get; set; }
        public string Power { get; set; }
        public string Location { get; set; }
        public string Unit { get; set; }
        public DateTime Timestamp { get; set; }
        public DateTime DataBaseTimestamp { get; set; }
        public bool IsActive { get; set; }
    }
}