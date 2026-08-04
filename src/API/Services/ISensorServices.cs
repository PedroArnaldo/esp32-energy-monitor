using API.Models;

namespace API.Services
{
    public interface ISensorServices
    {
        //Task<List<Sensor>> GetAllSensorsAsync();
       // Task<Sensor> GetSensorByIdAsync(int id);
        Task<Sensor> SaveSensorAsync(Sensor sensor);
        //Task<bool> UpdateSensorAsync(int id, Sensor sensor);
    }
}