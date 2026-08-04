using API.Data;
using API.Models;

namespace API.Services
{
    public class SensorServices : ISensorServices
    {
        private readonly AppDbContext _db;

        public SensorServices(AppDbContext db)
        {
            _db = db;
        }

        public async Task<Sensor> SaveSensorAsync(Sensor sensor)
        {
            _db.Sensors.Add(sensor);
            await _db.SaveChangesAsync();
            return sensor;
        }
    }
}